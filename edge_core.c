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
    char *config_path;
    int clen;
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &config_path, &clen) == FAILURE)
    {
        RETURN_FALSE;
    }
     
    //set root path
    set_root_path(config_path);
    spprintf(&EDGE_G(config_path), 0, "%s", config_path);
    
    //initialize config 
    zval *_config;
    _config = get_config_instance(EDGE_G(config_path) TSRMLS_CC);
    zend_update_static_property(edge_core_ce, ZEND_STRL("config"), _config);
    zval_ptr_dtor(&_config); 
      
    //initialize router
     zval *_router;
    _router = get_router_instance();
    zend_update_static_property(edge_core_ce, ZEND_STRL("router"), _router);
    zval_ptr_dtor(&_router);
       
    //initialize loader
    zval *_loader;
    _loader = get_loader_instance();
    zend_update_static_property(edge_core_ce, ZEND_STRL("loader"), _loader);
    zval_ptr_dtor(&_loader);
}

PHP_METHOD(Edge_Core, reg)
{
    char *key;
    int klen=0;
    zval *value=NULL;
    
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz", &key, &klen, &value) == FAILURE)
    {
        RETURN_FALSE;
    }
    
    if(klen == 0)
    {
        RETURN_ZVAL(EDGE_G(regs), 1, 0);
    }
    
    if(value == NULL)
    {
        zval **ppval;
        if(zend_hash_find(Z_ARRVAL_P(EDGE_G(regs)), key, klen+1, (void **)&ppval) == FAILURE)
        {
            RETURN_FALSE;
        }
        RETURN_ZVAL(*ppval, 1 , 0);
    }
         
    zval *tmp;
    MAKE_STD_ZVAL(tmp);
    *(tmp) = *(value);
    zval_copy_ctor(tmp);
    
    zend_hash_update(Z_ARRVAL_P(EDGE_G(regs)), key, klen+1, (void **)&tmp, sizeof(zval *), NULL);
    zval_ptr_dtor(&tmp);
    RETURN_NULL();
}

PHP_METHOD(Edge_Core, bootstrap)
{
    zval *action_ret =NULL;
    zval *_config;
    zval *_router;
    zval *_loader;
    _config = zend_read_static_property(edge_core_ce, ZEND_STRL("config"), 1 TSRMLS_DC);
    zval *config_data;
    config_data = zend_read_property(edge_config_ce, _config, ZEND_STRL("_data"), 1 TSRMLS_DC);

    _router = zend_read_static_property(edge_core_ce, ZEND_STRL("router"), 1 TSRMLS_DC);
    _loader = zend_read_static_property(edge_core_ce, ZEND_STRL("loader"), 1 TSRMLS_DC);
    //controller_home_path
    char *controller_path;
    char *controller_path_suffix = NULL;
    int clen;
    
    zval **ppzval;
    HashTable *cht;
    cht = Z_ARRVAL_P(config_data);
    if(zend_hash_find(Z_ARRVAL_P(config_data), "_controllers_home", strlen("_controllers_home")+1, (void **)&ppzval) == FAILURE)
    {
        //throw error with controller_home error
        RETURN_FALSE;
    }
    zval *dispathInfo;
    zval **_controller;
    dispathInfo = zend_read_property(edge_router_ce, _router, ZEND_STRL("dispatchInfo"), 1 TSRMLS_DC);
    if(zend_hash_find(Z_ARRVAL_P(dispathInfo), "controller", strlen("controller")+1, (void **)&_controller) == FAILURE)
    {
        //throw error with controller error
        RETURN_FALSE;
    }
    Z_STRVAL_PP(_controller)[0] = toupper(Z_STRVAL_PP(_controller)[0]);
    zend_class_entry *loader_ce;
    loader_ce = Z_OBJCE_P(_loader);
    zval *ret;

    char *controller_file;
    spprintf(&controller_file, 0, "%s%s",  Z_STRVAL_PP(_controller), "Controller");
    zval *z_controller_file;
    MAKE_STD_ZVAL(z_controller_file);
    ZVAL_STRING(z_controller_file, controller_file, 1);
    zend_call_method_with_2_params(&_loader, Z_OBJCE_P(_loader), NULL, "autoload", &ret, z_controller_file, *ppzval); 
    
    efree(controller_file);
    zval_ptr_dtor(&z_controller_file);

    if(!Z_BVAL_P(ret))
    {
        char *errorMsg = "Request path error\n";
        PHPWRITE(errorMsg, strlen(errorMsg));
        zval_ptr_dtor(&ret);
        RETURN_FALSE;
    }
    zval_ptr_dtor(&ret);

    zend_class_entry **ce = NULL;
    char *class_name = NULL;
    char *class_lowercase = NULL;
    int class_len;
    class_len = spprintf(&class_name, 0, "%s%s", Z_STRVAL_PP(_controller), "controller");
    class_lowercase = zend_str_tolower_dup(class_name, class_len);
    if(zend_hash_find(EG(class_table), class_lowercase, class_len + 1, (void **)&ce) == FAILURE || !ce)
    {
        //throw error with controller error
        char *errorMsg = "Can't find effective controller class name\n";
        PHPWRITE(errorMsg, strlen(errorMsg));
        efree(class_name);
        efree(class_lowercase);
        RETURN_FALSE;
    }
    efree(class_name);
    efree(class_lowercase);

    zval *controller_ce;
    MAKE_STD_ZVAL(controller_ce);
    object_init_ex(controller_ce, *ce);

    zval **_action;
    zval **mfptr;
    char *method_name;
    char *method_lowercase_name;
    int method_len;
    if(zend_hash_find(Z_ARRVAL_P(dispathInfo), "action", strlen("action")+1, (void **)&_action) == FAILURE)
    {
        char *errorMsg = "Internal error,action path error\n";
        PHPWRITE(errorMsg, strlen(errorMsg));
        zval_ptr_dtor(&controller_ce);
        RETURN_FALSE;
    }
    method_len = spprintf(&method_name, 0, "%s%s", Z_STRVAL_PP(_action), "Action");
    method_lowercase_name = zend_str_tolower_dup(method_name, method_len);

    if(zend_hash_find(&((*ce)->function_table), method_lowercase_name, method_len+1, (void **)&mfptr) == FAILURE)
    {
        char *errorMsg = "Can't find effective action name\n";
        PHPWRITE(errorMsg, strlen(errorMsg));
        zval_ptr_dtor(&controller_ce);
        efree(method_name);
        efree(method_lowercase_name);
        RETURN_FALSE;
    }else
    {

        //call the __construct function if exist
        zval **cfptr;
        if(zend_hash_find(&((*ce)->function_table), "__construct", strlen("__construct")+1, (void **)&cfptr) == SUCCESS)
        {
            zval *cretval;
            zend_call_method(&controller_ce, *ce, NULL, "__construct", strlen("__construct"), &cretval, 0, NULL, NULL TSRMLS_CC);
            zval_ptr_dtor(&cretval);
        }

        uint count = 0;
        zval ***call_args = NULL;

        zval *func_name;
        MAKE_STD_ZVAL(func_name);
        ZVAL_STRINGL(func_name, method_lowercase_name, method_len, 1);
        zend_call_method(&controller_ce, *ce, NULL, method_lowercase_name, method_len, &action_ret, 0, NULL, NULL TSRMLS_CC);
        
        if (EG(exception)) 
        {
            zval_ptr_dtor(&func_name);
            efree(method_name);
            efree(method_lowercase_name);
            zval_ptr_dtor(&controller_ce);
            RETURN_FALSE;
        }
        
        zval_ptr_dtor(&func_name);
    }
    efree(method_name);
    efree(method_lowercase_name);
    zval_ptr_dtor(&controller_ce);

    RETURN_ZVAL(action_ret, 1, 1);
}


EDGE_STARTUP_FUNCTION(core)
{
    zend_class_entry tmp_ce;
    INIT_CLASS_ENTRY(tmp_ce, "Edge_Core", edge_core_methods);
    edge_core_ce = zend_register_internal_class(&tmp_ce TSRMLS_CC);

    zend_declare_property_null(edge_core_ce, ZEND_STRL("config"), ZEND_ACC_PUBLIC | ZEND_ACC_STATIC TSRMLS_DC);
    zend_declare_property_null(edge_core_ce, ZEND_STRL("router"), ZEND_ACC_PUBLIC | ZEND_ACC_STATIC TSRMLS_DC);
    zend_declare_property_null(edge_core_ce, ZEND_STRL("loader"), ZEND_ACC_PUBLIC | ZEND_ACC_STATIC TSRMLS_DC);
    zend_declare_property_null(edge_core_ce, ZEND_STRL("instance"), ZEND_ACC_PUBLIC | ZEND_ACC_STATIC TSRMLS_DC);
    return SUCCESS;
}
