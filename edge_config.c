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


static int edge_ini_modify(zval *filename, long ctime TSRMLS_DC)
{
    zval n_ctime;
    php_stat(Z_STRVAL_P(filename), Z_STRLEN_P(filename), 7, &n_ctime TSRMLS_CC);
    if (Z_TYPE(n_ctime) != IS_TRUE && ctime != Z_LVAL(n_ctime)) {
        return Z_LVAL(n_ctime);
    }   
    return 0;
}

zval *get_config_instance(zval *config, const char *config_path) 
{
    object_init_ex(config, edge_config_ce);
     
    if(strcasecmp(config_path + strlen(config_path) - 3, "ini") == 0) {
        zval zv;
        ZVAL_STRING(&zv, config_path);
        edge_config_persistent(&zv, config);
        zval_ptr_dtor(&zv);
       
    } else {
        php_error_docref(NULL, E_WARNING, "[EDGE_WARING] Configuration file format is not correct");
    }
    
    zend_update_static_property(edge_config_ce, ZEND_STRL("instance"), config);
    return config;
}

static void edge_config_persistent(zval *filename, zval *obj)
{
    zval parse_function;
    zval bo;
    zval args[2];
    zval retval;
    ZVAL_BOOL(&bo, 1);
    ZVAL_STRING(&parse_function, "parse_ini_file");
    args[0] = *filename;
    args[1] = bo;
    call_user_function_ex(CG(function_table), NULL, &parse_function, &retval, 2, args, 1, NULL);
    zval_ptr_dtor(&parse_function);

    HashTable *persistent; 
    edge_config_cache *config_cache;
    zend_update_property(edge_config_ce, obj, ZEND_STRL("_data"), &retval);
    zval_ptr_dtor(&retval);
}

PHP_METHOD(Edge_Config, __construct)
{
    zval *_data;
    zval *object;
    zval *ini_config;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &_data) == FAILURE) {
        RETURN_FALSE;
    }
     /* 
    if (Z_TYPE_P(_data) == IS_STRING) {
        get_config_instance(Z_STRVAL_P(_data), object);
    } else if (Z_TYPE_P(_data) == IS_ARRAY) {
        zend_update_property(edge_config_ce, object, ZEND_STRL("_data"), _data);
    }*/
    object = getThis();
    if (Z_TYPE_P(_data) == IS_ARRAY) {
        zend_update_property(edge_config_ce, object, ZEND_STRL("_data"), _data);
    } else if (Z_TYPE_P(_data) == IS_STRING) {
        if(strcasecmp(Z_STRVAL_P(_data) + Z_STRLEN_P(_data) - 3, "ini") == 0) {
            edge_config_persistent(_data, object);
        } else {
            php_error_docref(NULL, E_WARNING, "[EDGE_WARING] Configuration file format is not correct");
            RETURN_FALSE;
        }
           
    }
    zend_update_static_property(edge_config_ce, ZEND_STRL("instance"), getThis());
}

PHP_METHOD(Edge_Config, get)
{
    zend_string *name;
    zval *data;

    data = zend_read_property(edge_config_ce, getThis(), ZEND_STRL("_data"), 1, NULL);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &name) == FAILURE) {
        RETURN_ZVAL(data, 1, 0);
    }

    zval *zv;
    HashTable *ht;
    if ((zv = zend_hash_find(Z_ARRVAL_P(data), name)) == NULL) {
        RETURN_FALSE;
    } else {
        RETURN_ZVAL(zv, 1, 0);
    }
}

PHP_METHOD(Edge_Config, set)
{
    zend_string *key;
    long klen=0;
    zval *value;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Sz", &key, &value) == FAILURE) {
        RETURN_FALSE;
    }

    zval *data;
    zval *overwrite;
    zval *object = getThis();
    zval **ppzval;

    data = zend_read_property(edge_config_ce, object, ZEND_STRL("_data"), 1, NULL);
    overwrite = zend_read_property(edge_config_ce, object, ZEND_STRL("_overwrite"), 1, NULL);

    if(Z_TYPE_P(overwrite)) {
        if(zend_hash_update(Z_ARRVAL_P(data), key, value) != NULL) {
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
    zend_declare_property_null(edge_config_ce, ZEND_STRL("instance"), ZEND_ACC_PUBLIC | ZEND_ACC_STATIC TSRMLS_DC);
    zend_declare_property_bool(edge_config_ce, ZEND_STRL("_overwrite"), 1, ZEND_ACC_PROTECTED TSRMLS_DC);
    zend_declare_property_null(edge_config_ce, ZEND_STRL("_data"), ZEND_ACC_PUBLIC TSRMLS_DC);
    return SUCCESS;
}







