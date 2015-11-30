#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "Zend/zend_interfaces.h" 
#include "php_edge.h"
#include "edge_config.h"
#include "edge_router.h"
#include "edge_loader.h"
#include "edge_controller.h"
#include "edge_core.h"

#define CONTROLLER_SUFFIX "Controller"
#define MODEL_SUFFIX "Model"

zend_class_entry *edge_core_ce;

zend_function_entry edge_core_methods[] = {
    PHP_ME(Edge_Core, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Edge_Core, bootstrap, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Edge_Core, reg, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    {NULL, NULL, NULL}
};

static void set_root_path(char *path)
{
    char *root_path;
    int clen;
    int flag = 0;
    clen = strlen(path);
    for(;clen>=0;clen--)
    {
        if('/' == path[clen])
            flag++;
        if(flag == 4)
            break;
    }
    root_path = estrndup(path, clen);
    spprintf(&EDGE_G(root_path), 0, "%s%s", root_path, "/");
    efree(root_path);
}

PHP_METHOD(Edge_Core, __construct)
{
    zval *config;
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &config) == FAILURE)
    {
        RETURN_FALSE;
    }
    //set root path
    set_root_path(Z_STRVAL_P(config));
    spprintf(&EDGE_G(config_path), 0, "%s", Z_STRVAL_P(config));
    //initialize config 
    zval _config;
    (void)get_config_instance(&_config, Z_STRVAL_P(config));
    zend_update_static_property(edge_core_ce, ZEND_STRL("config"), &_config);
    zval_ptr_dtor(&_config); 

    //initialize router
    zval _router;
    (void)get_router_instance(&_router);
    zend_update_static_property(edge_core_ce, ZEND_STRL("router"),&_router);
    zval_ptr_dtor(&_router);
       
    //initialize loader
    
    zval _loader;
    (void)get_loader_instance(&_loader);
    zend_update_static_property(edge_core_ce, ZEND_STRL("loader"),&_loader);
    zval_ptr_dtor(&_loader);
    
}

PHP_METHOD(Edge_Core, reg)
{
    
    zend_string *key = NULL;
    zval *value=NULL;
    
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|Sz", &key, &value) == FAILURE) {
        RETURN_FALSE;
    }
    
    if (key == NULL) {
        RETURN_ZVAL(&EDGE_G(regs), 1, 0);
    }
    
    if (value == NULL) {
        zval *pzval;
        if ((pzval=zend_hash_find(Z_ARRVAL(EDGE_G(regs)), key)) == NULL) {
            RETURN_FALSE;
        }
        RETURN_ZVAL(pzval, 1 , 0);
    }
         
    zval tmp;
    ZVAL_COPY(&tmp, value);
    zend_hash_update(Z_ARRVAL(EDGE_G(regs)), key, &tmp);
    RETURN_NULL();
}

PHP_METHOD(Edge_Core, bootstrap)
{
    zval *_config;
    zval *_router;
    zval *_loader;

    _config = zend_read_static_property(edge_core_ce, ZEND_STRL("config"), 1);
    zval *config_data;
    config_data = zend_read_property(edge_config_ce, _config, ZEND_STRL("_data"), 1, NULL);

    _router = zend_read_static_property(edge_core_ce, ZEND_STRL("router"), 1);
    _loader = zend_read_static_property(edge_core_ce, ZEND_STRL("loader"), 1);
    //controller_home_path
    char *controller_path;
    char *controller_path_suffix = NULL;
    int clen;
    
    zval *_controllers_home;
    if((_controllers_home=zend_hash_str_find(Z_ARRVAL_P(config_data), "_controllers_home", strlen("_controllers_home"))) == NULL) {
        php_error_docref(NULL, E_WARNING, "[EDGE_WARNING] Unable find _controllers_home");
        RETURN_FALSE;
    }

    zval *dispathInfo;
    zval *_controller;
    dispathInfo = zend_read_property(edge_router_ce, _router, ZEND_STRL("dispatchInfo"), 1, NULL);
    if((_controller=zend_hash_str_find(Z_ARRVAL_P(dispathInfo), "controller", strlen("controller"))) == NULL) {
        php_error_docref(NULL, E_WARNING, "[EDGE_WARNING] Unable find controller");
        RETURN_FALSE;
    }

    Z_STRVAL_P(_controller)[0] = toupper(Z_STRVAL_P(_controller)[0]);
    zval ret;

    char *controller_file;
    spprintf(&controller_file, 0, "%s%s",  Z_STRVAL_P(_controller), "Controller");
    zval z_controller_file;
    ZVAL_STRING(&z_controller_file, controller_file);
    zend_call_method_with_2_params(_loader, edge_loader_ce, NULL, "autoload", &ret, &z_controller_file, _controllers_home); 
    zval_ptr_dtor(&z_controller_file);

    if(Z_TYPE_P(&ret) == IS_FALSE) {
        php_error_docref(NULL, E_WARNING, "[EDGE_WARNING] Unable find controller by this request");
        zval_ptr_dtor(&ret);
        efree(controller_file);
        RETURN_FALSE;
    }
   
    if (!Z_ISUNDEF(ret)) {
        zval_ptr_dtor(&ret);
    }
    
    zend_class_entry *controller_ce = NULL;
    char *class_name = NULL;
    char *class_lowercase = NULL;
    int class_len = strlen(controller_file);
    
    class_lowercase = zend_str_tolower_dup(controller_file, class_len);
    if ((controller_ce=zend_hash_str_find_ptr(EG(class_table), class_lowercase, class_len)) == NULL) {
        php_error_docref(NULL, E_WARNING, "[EDGE_WARNING] Can't find effective controller class name");
        efree(class_lowercase);
        RETURN_FALSE;
    }
    efree(class_lowercase);
    efree(controller_file);

    zval *_action;
    if ((_action=zend_hash_str_find(Z_ARRVAL_P(dispathInfo), "action", strlen("action"))) == NULL) {
        php_error_docref(NULL, E_WARNING, "[EDGE_WARNING] Unable to find the action function in dispatch");
        RETURN_FALSE;
    }

    char *method_name;
    char *method_lowercase_name;
    int method_len;
    method_len = spprintf(&method_name, 0, "%s%s", Z_STRVAL_P(_action), "Action");
    method_lowercase_name = zend_str_tolower_dup(method_name, method_len);
    
    zval *mfptr;
    zval *cfptr;
    zval action_ret;
    zval controller_obj;
    object_init_ex(&controller_obj, controller_ce);
     
    if ((mfptr=zend_hash_str_find(&((controller_ce)->function_table), method_lowercase_name, method_len)) == NULL) {
        php_error_docref(NULL, E_WARNING, "[EDGE_WARNING] Unable to find effective action name");
        zval_ptr_dtor(&controller_obj);
        efree(method_name);
        efree(method_lowercase_name);
        RETURN_FALSE;
    } else {
        if ((cfptr=zend_hash_str_find(&((controller_ce)->function_table), "__construct", strlen("__construct"))) == NULL) {
            zval cretval;
            zend_call_method(&controller_obj, controller_ce, NULL, "__construct", strlen("__construct"), &cretval, 0, NULL, NULL);
            zval_ptr_dtor(&cretval);
        }

        zend_call_method(&controller_obj, controller_ce, NULL, method_lowercase_name, method_len, &action_ret, 0, NULL, NULL);
        
        if (EG(exception)) {
            efree(method_name);
            efree(method_lowercase_name);
            zval_ptr_dtor(&controller_obj);
            RETURN_FALSE;
        }
    }
    //php_var_dump(&controller_obj, 1);
    efree(method_name);
    efree(method_lowercase_name);
    zval_ptr_dtor(&controller_obj);
    RETURN_ZVAL(&action_ret, 1, 1);
}


EDGE_STARTUP_FUNCTION(core)
{
    zend_class_entry tmp_ce;
    INIT_CLASS_ENTRY(tmp_ce, "Edge_Core", edge_core_methods);
    edge_core_ce = zend_register_internal_class_ex(&tmp_ce, NULL);

    zend_declare_property_null(edge_core_ce, ZEND_STRL("config"), ZEND_ACC_PUBLIC | ZEND_ACC_STATIC TSRMLS_DC);
    zend_declare_property_null(edge_core_ce, ZEND_STRL("router"), ZEND_ACC_PUBLIC | ZEND_ACC_STATIC TSRMLS_DC);
    zend_declare_property_null(edge_core_ce, ZEND_STRL("loader"), ZEND_ACC_PUBLIC | ZEND_ACC_STATIC TSRMLS_DC);
    zend_declare_property_null(edge_core_ce, ZEND_STRL("instance"), ZEND_ACC_PUBLIC | ZEND_ACC_STATIC TSRMLS_DC);
    return SUCCESS;
}
