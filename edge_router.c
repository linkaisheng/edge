#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_edge.h"
#include "edge_router.h"
#include "edge_config.h"

zend_class_entry *edge_router_ce;

zend_function_entry edge_router_methods[] = {
    PHP_ME(Edge_Router, __construct, NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

zval * edge_request_query(int type, char *name)
{
    zval **rdata;
    zend_bool   jit_initialization = PG(auto_globals_jit);
    switch(type)
    {
        case EDGE_REQUEST_VARS_GET:
        case EDGE_REQUEST_VARS_POST:
        case EDGE_REQUEST_VARS_COOKIE:
            rdata = &PG(http_globals)[type];
            break;
        case EDGE_REQUEST_VARS_SERVER:
            if(jit_initialization)
            {
                zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC);
            }
            (void)zend_hash_find(&EG(symbol_table), ZEND_STRS("_SERVER"), (void **)&rdata);
        default:
            break;
    }
    zval **ppzval;
    int len;
    len = strlen(name);
    if(len == 0)
    {
       Z_ADDREF_P(*rdata);
       return *rdata;
    }
    if(rdata)
    {
       if( zend_hash_find(Z_ARRVAL_PP(rdata), name, len+1, (void **)&ppzval) == FAILURE)
       {
           zval *empty;
           MAKE_STD_ZVAL(empty);
           ZVAL_NULL(empty);
           return empty;
       }
       Z_ADDREF_P(*ppzval);
       return *ppzval;
    }else{
        zval *empty;
        MAKE_STD_ZVAL(empty);
        ZVAL_NULL(empty);
        return empty;
    }

}

static void set_base_home(const char *interface_path)
{
    zval *config_instance;
    config_instance = zend_read_static_property(edge_config_ce,  ZEND_STRL("t_instance"), 1 TSRMLS_DC);
    char *_controllers_home;
    char *_modeles_home;
    char *_views_home;
    spprintf(&_controllers_home, 0, "/modules/%s/controllers/", interface_path);
    spprintf(&_modeles_home, 0, "/modules/%s/models/", interface_path);
    spprintf(&_views_home, 0, "/modules/%s/views/", interface_path);

    zval *z_controllers_home;
    zval *z_models_home;
    zval *z_views_home;

    MAKE_STD_ZVAL(z_controllers_home);
    MAKE_STD_ZVAL(z_models_home);
    MAKE_STD_ZVAL(z_views_home);

    ZVAL_STRING(z_controllers_home, _controllers_home, 1);
    ZVAL_STRING(z_models_home, _modeles_home, 1);
    ZVAL_STRING(z_views_home, _views_home, 1);
    
    zval *configs;
    char *m="_models_home";
    char *v="_views_home";
    char *c="_controllers_home";
    configs = zend_read_property(edge_config_ce, config_instance, ZEND_STRL("_data"), 1 TSRMLS_DC);
    zend_hash_update(Z_ARRVAL_P(configs), "_models_home", sizeof("_models_home"), (void **)&z_models_home, sizeof(zval *), NULL);
    zend_hash_update(Z_ARRVAL_P(configs), v, strlen(v)+1, (void **)&z_views_home, sizeof(zval *), NULL);
    zend_hash_update(Z_ARRVAL_P(configs), c, strlen(c)+1, (void **)&z_controllers_home, sizeof(zval *), NULL);
    
    efree(_controllers_home);
    efree(_modeles_home);
    efree(_views_home);
}

static void dispatch(zval *obj) 
{
    zval *path_info;
    path_info = edge_request_query(EDGE_REQUEST_VARS_SERVER, "PATH_INFO");
    if(Z_TYPE_P(path_info) == IS_NULL)
    {
        //throw error here..
        zval_ptr_dtor(&path_info);
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "path info error");
        zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "path info error");
        return ;
    }
    char *path_info_string;
    char *ptr;
    //char *token[4] = {NULL, NULL, NULL, NULL};
    char *token[1024] = {0};
    char *delim = "/";
    char *jt = NULL;
    int offset = 1;
    path_info_string = estrndup(Z_STRVAL_P(path_info), Z_STRLEN_P(path_info));
    if((jt = php_strtok_r(path_info_string, delim, &ptr)) != NULL)
    {
        token[0] = jt;
        while((jt = php_strtok_r(NULL, delim, &ptr)) != NULL)
        {
            token[offset] = jt;
            offset++;
        }
    }

    if(offset < 2)
    {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "path info rule error");
        zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "path info rule error");
        zval_ptr_dtor(&path_info);
        efree(path_info_string);
        return ;
    }

    char tmp_interface_path[1024] = {0};
    char *t_interface_path;
    t_interface_path = tmp_interface_path;

    int module_levels = 0;
    for(;module_levels < offset -2; module_levels++)
    {
        strcpy(t_interface_path, token[module_levels]);
        t_interface_path += strlen(token[module_levels]);
        strcpy(t_interface_path, "/");
        t_interface_path +=1;
    }
    
    char *base_module;
    char *module;
    char *controller;
    char *action;
    char *interface_path;

    LPS(token[offset-2], controller)
    LPS(token[offset-1], action)
    spprintf(&interface_path, 0, "%s", tmp_interface_path);
    zval *z_controller;
    zval *z_action;
    zval *z_interface_path;

    MAKE_STD_ZVAL(z_controller);
    MAKE_STD_ZVAL(z_action);
    MAKE_STD_ZVAL(z_interface_path);

    ZVAL_STRING(z_controller, controller, 1);
    ZVAL_STRING(z_action, action, 1);
    ZVAL_STRING(z_interface_path, interface_path, 1);

    zval *dispatchInfo;
    dispatchInfo = zend_read_property(edge_router_ce, obj, ZEND_STRL("dispatchInfo"), 1 TSRMLS_DC);
    zend_hash_update(Z_ARRVAL_P(dispatchInfo), "controller", strlen("controller")+1, (void **)&z_controller, sizeof(zval *), NULL);
    zend_hash_update(Z_ARRVAL_P(dispatchInfo), "action", strlen("action")+1, (void **)&z_action, sizeof(zval *), NULL);
    zend_hash_update(Z_ARRVAL_P(dispatchInfo), "interface_path", strlen("interface_path")+1, (void **)&z_interface_path, sizeof(zval *), NULL);

    set_base_home(interface_path);
    zval_ptr_dtor(&path_info);
    efree(interface_path);
    efree(controller);
    efree(action);
    efree(path_info_string);
}

static void init_ce(zval *obj)
{
    zval *dispatchInfo;
    MAKE_STD_ZVAL(dispatchInfo);
    array_init(dispatchInfo);
    add_property_zval_ex(obj, ZEND_STRS("dispatchInfo"), dispatchInfo);
    zval_ptr_dtor(&dispatchInfo);
}

zval * get_router_instance()
{
    zval *router_instance;
    MAKE_STD_ZVAL(router_instance);
    object_init_ex(router_instance, edge_router_ce);
    init_ce(router_instance);
    dispatch(router_instance);
    return router_instance;
}

PHP_METHOD(Edge_Router, __construct)
{
    /*
    zval *dispatchInfo;
    MAKE_STD_ZVAL(dispatchInfo);
    array_init(dispatchInfo);
    add_property_zval_ex(getThis(), ZEND_STRS("dispatchInfo"), dispatchInfo);
    dispatch(getThis());
    zval_ptr_dtor(&dispatchInfo);
    */
    zval *obj = getThis();
    init_ce(obj);
    dispatch(obj);
}

EDGE_STARTUP_FUNCTION(router)
{
    zend_class_entry tmp_ce;
    INIT_CLASS_ENTRY(tmp_ce, "Edge_Router", edge_router_methods);
    edge_router_ce = zend_register_internal_class(&tmp_ce TSRMLS_CC);
    return SUCCESS;
}
