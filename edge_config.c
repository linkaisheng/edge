#ifdef HAVE_CONFIG_H                                                    
#include "config.h"                                                     
#endif 

#include "php.h"                                                        
#include "php_ini.h" 

#include "php_edge.h"
#include "edge_config.h"


zend_class_entry *edge_config_ce;

zend_function_entry edge_config_methods[] = {
    PHP_ME(Edge_Config, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Edge_Config, get, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Edge_Config, set, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Edge_Config, reg, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Edge_Config, merge, NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

static void edge_config_cache_dtor(edge_config_cache **cache)
{
    if(*cache)
    {
        zend_hash_destroy((*cache)->config);
        pefree((*cache)->config, 1);
        pefree(*cache, 1);
    }   
}

static void edge_config_zval_dtor(zval **value)
{
    if(*value)
    {
        switch(Z_TYPE_PP(value))
        {
            case IS_STRING:
            case IS_CONSTANT:
                CHECK_ZVAL_STRING(*value);
                pefree((*value)->value.str.val, 1);
                pefree(*value, 1);
                break;
            case IS_ARRAY:
            case IS_CONSTANT_ARRAY:
                zend_hash_destroy((*value)->value.ht);
                pefree((*value)->value.ht, 1);
                pefree(*value, 1);
                break;
        }
    }
}

static int edge_ini_modify(zval *filename, long ctime TSRMLS_DC)
{
    zval n_ctime;
    php_stat(Z_STRVAL_P(filename), Z_STRLEN_P(filename), 7, &n_ctime TSRMLS_CC);
    if (Z_TYPE(n_ctime) != IS_BOOL && ctime != Z_LVAL(n_ctime)) 
    {
        return Z_LVAL(n_ctime);
    }   
    return 0;
}

static void edge_config_copy(HashTable *dst, HashTable *src TSRMLS_DC)
{
    zval **ppzval;
    char *key;
    uint keylen;
    ulong idx;

    for(zend_hash_internal_pointer_reset(src); zend_hash_has_more_elements(src) == SUCCESS; zend_hash_move_forward(src))
    {
        if(zend_hash_get_current_key_ex(src, &key, &keylen, &idx, 0, NULL) == HASH_KEY_IS_LONG)
        {
            zval *tmp;
            if(zend_hash_get_current_data(src, (void **)&ppzval) == FAILURE)
            {
                continue;
            }

            tmp = edge_ini_array_copy(*ppzval TSRMLS_CC);

            if(tmp)
            {
                zend_hash_index_update(dst, idx, (void **)&tmp, sizeof(zval *), NULL);
            }
        }else{
            zval *tmp;
            if(zend_hash_get_current_data(src, (void **)&ppzval) == FAILURE)
            {
                continue;
            }

            tmp = edge_ini_array_copy(*ppzval TSRMLS_CC);

            if(tmp)
            {
                zend_hash_update(dst, key, keylen, (void **)&tmp, sizeof(zval *), NULL);
            }
        }
        
    }

}

static void edge_config_copy_losable(HashTable *ldst, HashTable *src TSRMLS_DC) {
	zval **ppzval, *tmp;
	char *key;
	ulong idx;
	uint keylen;

	for(zend_hash_internal_pointer_reset(src);
			zend_hash_has_more_elements(src) == SUCCESS;
			zend_hash_move_forward(src)) {

		if (zend_hash_get_current_key_ex(src, &key, &keylen, &idx, 0, NULL) == HASH_KEY_IS_LONG) 
        {
			if (zend_hash_get_current_data(src, (void**)&ppzval) == FAILURE) 
            {
				continue;
			}

			tmp = edge_config_ini_zval_losable(*ppzval TSRMLS_CC);
			zend_hash_index_update(ldst, idx, (void **)&tmp, sizeof(zval *), NULL);

		} else 
        {
			if (zend_hash_get_current_data(src, (void**)&ppzval) == FAILURE) 
            {
				continue;
			}

			tmp = edge_config_ini_zval_losable(*ppzval TSRMLS_CC);
			zend_hash_update(ldst, key, keylen, (void **)&tmp, sizeof(zval *), NULL);
		}
	}
}

static void edge_config_persistent(zval *filename, zval *obj TSRMLS_DC)
{
    zval *config;
    HashTable *persistent; 
    edge_config_cache *config_cache;

    config = edge_parse_ini_file(filename);
    add_property_zval_ex(obj, ZEND_STRS("_data"), config); 

    if(!EDGE_G(configs))
    {
        EDGE_G(configs) = (HashTable *)pemalloc(sizeof(HashTable), 1);
        zend_hash_init(EDGE_G(configs), 8, NULL, (dtor_func_t) edge_config_cache_dtor, 1);
        config_cache = (edge_config_cache *)pemalloc(sizeof(edge_config_cache), 1);
        if(!config_cache)
        {
            return ;
        }

        persistent = (HashTable *) pemalloc(sizeof(HashTable), 1);
        zend_hash_init(persistent, zend_hash_num_elements(Z_ARRVAL_P(config)), NULL, (dtor_func_t)edge_config_zval_dtor, 1);

        edge_config_copy(persistent, Z_ARRVAL_P(config) TSRMLS_CC);
       
        config_cache->config = persistent;  
        config_cache->ctime = edge_ini_modify(filename, 0);

        char *key;
        long klen;
        klen = spprintf(&key, 0, "%s", Z_STRVAL_P(filename));
        zend_hash_update(EDGE_G(configs), key, klen + 1, (void **)&config_cache, sizeof(edge_config_cache *), NULL);
        efree(key);
    }
    zval_ptr_dtor(&config);
}

static void edge_config_by_persistent(zval *filename, zval *obj TSRMLS_DC)
{
    zval *config;
    edge_config_cache **config_cache;
    
    if(EDGE_G(configs))
    {
        char *key;
        long klen;
        klen = spprintf(&key, 0, "%s", Z_STRVAL_P(filename));
        if(zend_hash_find(EDGE_G(configs), key, klen+1, (void **)&config_cache) == SUCCESS)
        {
            zval *_config_data;
            long ctime;
            ctime = (*config_cache)->ctime;
            
            if(!edge_ini_modify(filename, ctime))
            {
                zval *props;
                MAKE_STD_ZVAL(props);
                array_init(props);
                edge_config_copy_losable(Z_ARRVAL_P(props), (*config_cache)->config TSRMLS_CC);
                add_property_zval_ex(obj, ZEND_STRS("_data"), props);
            }else{
                edge_config_persistent(filename, obj TSRMLS_CC);
            }
        }else{
            edge_config_persistent(filename, obj TSRMLS_CC);
        }
        efree(key);
    }
    
}

zval *edge_ini_array_copy(zval *zv TSRMLS_DC)
{
    zval *ret;
    ret = (zval *)pemalloc(sizeof(zval), 1);
    INIT_PZVAL(ret);

    switch(zv->type)
    {   
        case IS_CONSTANT:
        case IS_STRING:
            CHECK_ZVAL_STRING(zv);
            Z_TYPE_P(ret) = IS_STRING;
            ret->value.str.val = pestrndup((zv)->value.str.val, (zv)->value.str.len, 1); 
            ret->value.str.len = (zv)->value.str.len;
            break;
        case IS_ARRAY:
        case IS_CONSTANT_ARRAY:
            {   
                HashTable *tmp_ht, *local_ht = zv->value.ht;
                tmp_ht = (HashTable *) pemalloc (sizeof(HashTable), 1); 
                if(!tmp_ht){
                    return; 
                } 

                zend_hash_init(tmp_ht, zend_hash_num_elements(local_ht), NULL, (dtor_func_t)edge_config_zval_dtor, 1); 
                edge_config_copy(tmp_ht, local_ht TSRMLS_CC);
                Z_TYPE_P(ret) = IS_ARRAY;
                ret->value.ht = tmp_ht;
            }
            break;
    }
    return ret;
}

static zval * edge_config_ini_zval_losable(zval *zvalue TSRMLS_DC) {
	zval *ret;
	MAKE_STD_ZVAL(ret);
	switch (zvalue->type) {
		case IS_RESOURCE:
		case IS_OBJECT:
			break;
		case IS_BOOL:
		case IS_LONG:
		case IS_NULL:
			break;
		case IS_CONSTANT:
		case IS_STRING:
			CHECK_ZVAL_STRING(zvalue);
			ZVAL_STRINGL(ret, zvalue->value.str.val, zvalue->value.str.len, 1);
			break;
		case IS_ARRAY:
		case IS_CONSTANT_ARRAY: {
			HashTable *original_ht = zvalue->value.ht;
			array_init(ret);
			edge_config_copy_losable(Z_ARRVAL_P(ret), original_ht TSRMLS_CC);
		}
			break;
	}

	return ret;
}


zval *edge_parse_ini_file(zval *zv)
{
    zval *parse_function;
    zval **args[2];
    zval *ret;
    zval *bo;
    MAKE_STD_ZVAL(bo);
    ZVAL_BOOL(bo, 1);
    MAKE_STD_ZVAL(parse_function);
    ZVAL_STRING(parse_function, "parse_ini_file", 1);
    args[0] = &zv;
    args[1] = &bo;
    call_user_function_ex(CG(function_table), NULL, parse_function, &ret, 2, args, 0, NULL TSRMLS_CC);
    zval_ptr_dtor(&parse_function);
    zval_ptr_dtor(&bo);
    return ret;
}

zval * get_config_instance(char *config_path TSRMLS_DC)
{
    zval *config_instance;
    zval *zconfig;

    MAKE_STD_ZVAL(config_instance);
    object_init_ex(config_instance, edge_config_ce);

    MAKE_STD_ZVAL(zconfig);
    ZVAL_STRING(zconfig, config_path, 1);

    char *fileType;
    fileType = config_path + strlen(config_path) -3; 
    if(strcasecmp(fileType, "ini") == 0)
    {   
        /*persistent in here*/
        if(!EDGE_G(configs))
        {   
            edge_config_persistent(zconfig, config_instance TSRMLS_CC);
        }else{
            edge_config_by_persistent(zconfig, config_instance TSRMLS_CC);
        } 
    }  
    zend_update_static_property(edge_config_ce, ZEND_STRL("t_instance"), config_instance);
    zval_ptr_dtor(&zconfig);
    return config_instance;
}


PHP_METHOD(Edge_Config, __construct)
{
    zval *_data;
    zval *object;
    zval *ini_config;
    if(zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "z", &_data) == FAILURE)
    {
        MAKE_STD_ZVAL(_data);
        array_init(_data);
    }
    object = getThis();
    if(Z_TYPE_P(_data) == IS_ARRAY)
    {
        Add_property_zval_ex(object, ZEND_STRS("_data"), _data); 
    }else if(Z_TYPE_P(_data) == IS_STRING)
    {
        char *fileType;
        fileType = Z_STRVAL_P(_data) + Z_STRLEN_P(_data) -3;
        if(strcasecmp(fileType, "ini") == 0)
        {
            /*persistent in here*/
            if(!EDGE_G(configs))
            {
                edge_config_persistent(_data, getThis());
            }else
            {
                edge_config_by_persistent(_data, getThis());
            }

        }
    }

    zend_update_property(edge_config_ce, object, ZEND_STRL("instance"), object);
    zend_update_static_property(edge_config_ce, ZEND_STRL("t_instance"), object);
    zval_ptr_dtor(&_data);
}

PHP_METHOD(Edge_Config, get)
{
    char *name;
    long len = 0;
    zval *data;

    zend_class_entry *ce;
    ce = Z_OBJCE_P(getThis());
    data = zend_read_property(ce, getThis(), ZEND_STRL("_data"), 1 TSRMLS_DC);
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &name, &len) == FAILURE)
    {
        return ;
    }

    if(len == 0){
        RETURN_ZVAL(data, 1, 0);
    }else{
        zval **ppzval;
        HashTable *ht;
        ht = Z_ARRVAL_P(data);
        if(zend_hash_find(ht, name, strlen(name)+1, (void **)&ppzval) == FAILURE)
        {
            RETURN_FALSE;
        }else{
            RETURN_ZVAL(*ppzval, 1, 0);
        }
    }
}

PHP_METHOD(Edge_Config, set)
{
    char *key;
    long klen=0;
    zval *value;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &key, &klen, &value) == FAILURE)
    {
        return ;
    }

    if(klen == 0)
    {
        RETURN_FALSE;
    }

    zval *data;
    zval *overwrite;
    zval **ppzval;
    zend_class_entry *ce;

    ce = Z_OBJCE_P(getThis());
    data = zend_read_property(ce, getThis(), ZEND_STRL("_data"), 1 TSRMLS_DC);
    overwrite = zend_read_property(ce, getThis(), ZEND_STRL("_overwrite"), 1 TSRMLS_DC);

    if(Z_BVAL_P(overwrite))
    {
        if(zend_hash_update(Z_ARRVAL_P(data), key, strlen(key)+1, (void **)&value, sizeof(zval *), NULL) == SUCCESS)
        {
            Z_ADDREF_P(value);
            RETURN_TRUE;
        }
    }

    RETURN_FALSE;
}

PHP_METHOD(Edge_Config, reg)
{

}

PHP_METHOD(Edge_Config, merge)
{

}


EDGE_STARTUP_FUNCTION(config){
    zend_class_entry tmp_ce;
    INIT_CLASS_ENTRY(tmp_ce, "Edge_Config", edge_config_methods);
    edge_config_ce = zend_register_internal_class(&tmp_ce TSRMLS_CC);

    zend_declare_property_string(edge_config_ce, ZEND_STRL("config"), "defualt", ZEND_ACC_PUBLIC TSRMLS_DC);
    zend_declare_property_bool(edge_config_ce, ZEND_STRL("_overwrite"), 1, ZEND_ACC_PROTECTED TSRMLS_DC);
    zend_declare_property_null(edge_config_ce, ZEND_STRL("instance"), ZEND_ACC_PUBLIC TSRMLS_DC);
    zend_declare_property_null(edge_config_ce, ZEND_STRL("t_config_config"), ZEND_ACC_PUBLIC TSRMLS_DC);
    zend_declare_property_null(edge_config_ce, ZEND_STRL("t_instance"), ZEND_ACC_PUBLIC | ZEND_ACC_STATIC TSRMLS_DC);
    return SUCCESS;
}







