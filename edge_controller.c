#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "Zend/zend_interfaces.h"
#include "php_edge.h"
#include "edge_config.h"
#include "edge_router.h"
#include "edge_loader.h"
#include "edge_core.h"
#include "edge_controller.h"

zend_class_entry *edge_controller_ce;

ZEND_BEGIN_ARG_INFO_EX(edge_controller_get_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, key_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(edge_controller_check_login, 0, 0, 3)
    ZEND_ARG_INFO(0, chktype)
    ZEND_ARG_INFO(1, uin)
    ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()


zend_function_entry edge_controller_methods[] = {
    PHP_ME(Edge_Controller, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Edge_Controller, get, edge_controller_get_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(Edge_Controller, model, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Edge_Controller, post, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Edge_Controller, result, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Edge_Controller, check_login, edge_controller_check_login, ZEND_ACC_PUBLIC)
    PHP_MALIAS(Edge_Controller, __get, get, edge_controller_get_arginfo, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};


PHP_METHOD(Edge_Controller, __construct)
{

}

PHP_METHOD(Edge_Controller, get)
{
    zend_string *name;
    int nlen = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &name) == FAILURE) {
        RETURN_FALSE;
    }
    zval *data = NULL;
    data = edge_request_query(EDGE_REQUEST_VARS_GET, ZSTR_VAL(name));
    if (data != NULL) {
        RETURN_ZVAL(data, 1, 1);
    }
    
    if (ZSTR_LEN(name) != 0) {
        if (strncmp("model", ZSTR_VAL(name), ZSTR_LEN(name)) == 0) {
            zval arg;
            zval ret;
            
            zend_class_entry *ce; 
            ce = Z_GET_OBJ_P(getThis());
             
            char *controller_prefix;
            controller_prefix = estrndup(ZSTR_VAL(ce->name), ZSTR_LEN(ce->name)- strlen("Controller"));
            char *model_class_name;
            spprintf(&model_class_name, 0, "%s", controller_prefix);
            ZVAL_STRING(&arg, model_class_name);
            
            zend_call_method_with_1_params(getThis(), edge_controller_ce, NULL, "model", &ret, &arg);
            efree(model_class_name);
            zval_ptr_dtor(&arg);
            
            if (Z_TYPE(ret) == IS_FALSE) {
                RETURN_ZVAL(&ret, 1, 1);
            }

            efree(controller_prefix);
            RETURN_ZVAL(&ret, 1, 1);
        } else {
            char *mg_class_name = NULL;
            ZSTR_VAL(name)[0] = toupper(ZSTR_VAL(name)[0]);
            int mg_class_len = spprintf(&mg_class_name, 0, "If_%s", ZSTR_VAL(name));
           
            zend_string *class_name;
            class_name = zend_string_init(mg_class_name, mg_class_len, 0);

            zend_class_entry *mg_ce;
            if((mg_ce = zend_lookup_class(class_name)) == NULL) {
                efree(mg_class_name);
                zend_string_release(class_name);
                RETURN_FALSE; 
            }
            efree(mg_class_name);
            zend_string_release(class_name);
           
            zval mg_obj;
            object_init_ex(&mg_obj, mg_ce);
            RETURN_ZVAL(&mg_obj, 1, 1);
        }
    }
    //RETURN_ZVAL(data, 1, 1);
}

PHP_METHOD(Edge_Controller, check_login)
{
    /*
    zval *chktype, *uin, *key;
    int had_params = 1;
    if(zend_parse_parameters(ZEND_NUM_ARGS(), "zzz", &chktype, &uin, &key) == FAILURE) {
        MAKE_STD_ZVAL(chktype);
        MAKE_STD_ZVAL(uin);
        MAKE_STD_ZVAL(key);

        ZVAL_NULL(chktype);
        ZVAL_NULL(uin);
        ZVAL_NULL(key);
        had_params = 0;
        //RETURN_FALSE;
    }

    zend_class_entry **login_ce;
    if(zend_lookup_class("If_Login", strlen("If_Login"), &login_ce TSRMLS_CC) == FAILURE)
    {
        RETURN_FALSE; 
    }

    zval *login_obj;
    MAKE_STD_ZVAL(login_obj);
    object_init_ex(login_obj, *login_ce);
    
    zval **args[3];
    zval *function_name;
    zval *ret = NULL;
    MAKE_STD_ZVAL(function_name);
    ZVAL_STRING(function_name, "check_login", 1);

    args[0] = &chktype;
    args[1] = &uin;
    args[2] = &key;
    
    if(call_user_function_ex(&((*login_ce)->function_table), &login_obj, function_name, &ret, 3, args, 0, NULL TSRMLS_CC )== FAILURE)
    {
        zval_ptr_dtor(&function_name);
        zval_ptr_dtor(&login_obj);
        RETURN_FALSE;
    }
    
    zval_ptr_dtor(&function_name);
    zval_ptr_dtor(&login_obj);
   
    if(had_params == 0) {
        zval_ptr_dtor(&chktype);
        zval_ptr_dtor(&uin);
        zval_ptr_dtor(&key);
    }

    RETURN_ZVAL(ret, 1, 1);
    */
    /*
    if(Z_TYPE_P(ret) == IS_BOOL) 
    {
        RETURN_FALSE;
    }

    zval *fret;
    MAKE_STD_ZVAL(fret);
    ZVAL_STRING(fret, Z_STRVAL_P(uin), 1);

    array_init(return_value);
    add_assoc_string(return_value, "nickname", Z_STRVAL_P(fret), 1);
    zval_ptr_dtor(&fret);
    */
}


PHP_METHOD(Edge_Controller, post)
{
    zend_string *name;
    int nlen;
    if(zend_parse_parameters(ZEND_NUM_ARGS(), "S", &name) == FAILURE) {
        RETURN_FALSE;
    }
    zval * data;
    data = edge_request_query(EDGE_REQUEST_VARS_POST, ZSTR_VAL(name));
    if (data) {
        RETURN_ZVAL(data, 1, 1);
    }else {
        RETURN_FALSE;
    }
}

PHP_METHOD(Edge_Controller, model)
{
    zend_string *model_name = NULL;
    zend_string *model_dir = NULL;
    zend_string *model_class_name = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|S", &model_name, &model_dir) == FAILURE) {
        RETURN_FALSE;
    }

    char *class_name = NULL;
    spprintf(&class_name, 0, "%s%s", ZSTR_VAL(model_name), "Model");
    model_class_name = zend_string_init(class_name, strlen(class_name), 0); 
    
    zval *z_obj;
    if ((z_obj=zend_hash_find(Z_ARRVAL(EDGE_G(regs)), model_class_name))) {
        zend_string_release(model_class_name); 
        efree(class_name);
        RETURN_ZVAL(z_obj, 1, 0);
    }
    zval *config;
    zval *config_data;
    config = zend_read_static_property(edge_core_ce, ZEND_STRL("config"), 1);
    config_data = zend_read_property(edge_config_ce, config, ZEND_STRL("_data"), 1, NULL);
    
    zval *models_home_p;
    if ((models_home_p=zend_hash_str_find(Z_ARRVAL_P(config_data), "_models_home", strlen("_models_home"))) == NULL) {
        zend_string_release(model_class_name); 
        efree(class_name);
        RETURN_FALSE;
    }
    
    zval z_model_name;
    ZVAL_STRING(&z_model_name, ZSTR_VAL(model_class_name));

    zval *loader;
    zval ret;
    loader = zend_read_static_property(edge_core_ce, ZEND_STRL("loader"), 1);
    zend_call_method_with_2_params(loader, Z_GET_OBJ_P(loader), NULL, "autoload", &ret, &z_model_name, models_home_p);
    zval_ptr_dtor(&z_model_name);
    if(Z_TYPE(ret) == IS_FALSE) {
        php_error_docref(NULL, E_WARNING, "[EDGE_WARNING] Unable load %s/%s class", Z_STRVAL_P(models_home_p), ZSTR_VAL(model_name));
        zval_ptr_dtor(&ret);
        zend_string_release(model_class_name); 
        efree(class_name);
        RETURN_FALSE;
    }
    zval_ptr_dtor(&ret);
    
    zend_class_entry *model_ce;
    if ((model_ce = zend_lookup_class(model_class_name)) == NULL) {
        php_error_docref(NULL, E_WARNING, "[EDGE_WARNING] Unable find %s class", ZSTR_VAL(model_class_name));
        zend_string_release(model_class_name); 
        efree(class_name);
        RETURN_FALSE;
    }

    zval model_obj;
    object_init_ex(&model_obj, model_ce);
    
    zval *cfptr;
    if ((cfptr=zend_hash_str_find(&((model_ce)->function_table), "__construct", strlen("__construct")))) {
        zval cretval;
        zend_call_method(&model_obj, model_ce, NULL, "__construct", strlen("__construct"), &cretval, 0, NULL, NULL);
        zval_ptr_dtor(&cretval);
    }

    zend_hash_update(Z_ARRVAL(EDGE_G(regs)), model_class_name, &model_obj);
    zend_string_release(model_class_name);
    efree(class_name); 
    RETURN_ZVAL(&model_obj, 1, 0);
    
}

PHP_METHOD(Edge_Controller, result)
{
    zval *data;
    zend_string *msg;
    int mlen;
    long code;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l|zS", &code, &data, &msg) == FAILURE) {
        RETURN_FALSE;
    }

    Z_ADDREF_P(data);
    array_init(return_value);  
    add_assoc_long(return_value, "code", code);
    add_assoc_string(return_value, "msg", ZSTR_VAL(msg));
    add_assoc_zval(return_value, "data", data);
}
EDGE_STARTUP_FUNCTION(controller)
{
    zend_class_entry tmp_ce;
    INIT_CLASS_ENTRY(tmp_ce, "Edge_Controller", edge_controller_methods);
    edge_controller_ce = zend_register_internal_class(&tmp_ce);
    return SUCCESS; 
}
