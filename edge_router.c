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
    zval *rdata = NULL;
    zend_bool   jit_initialization = PG(auto_globals_jit);
    switch(type)
    {
#if EDGE_CLI
        case EDGE_REQUEST_VARS_GET:
            rdata = zend_hash_str_find(&EG(symbol_table), ZEND_STRL("_GET"));
            break;
        case EDGE_REQUEST_VARS_POST:
            rdata = zend_hash_str_find(&EG(symbol_table), ZEND_STRL("_POST"));
            break;
        case EDGE_REQUEST_VARS_COOKIE:
            rdata = zend_hash_str_find(&EG(symbol_table), ZEND_STRL("_COOKIE"));
            break;
#else
        case EDGE_REQUEST_VARS_GET:
        case EDGE_REQUEST_VARS_POST:
        case EDGE_REQUEST_VARS_COOKIE:
            rdata = &PG(http_globals)[type];
            break;
#endif
        case EDGE_REQUEST_VARS_SERVER:
            if (jit_initialization) {
                zend_string *server_str = zend_string_init("_SERVER", sizeof("_SERVER") - 1, 0); 
                zend_is_auto_global(server_str);
                zend_string_release(server_str);
            }
            rdata = zend_hash_str_find(&EG(symbol_table), ZEND_STRL("_SERVER"));
            break;
        default:
            break;
    }

    if (strlen(name) == 0) {
       Z_ADDREF_P(rdata);
       return rdata;
    }

    zval *pzval;
    if (rdata) {
       if (pzval=zend_hash_str_find(Z_ARRVAL_P(rdata), name, strlen(name))) {
           Z_TRY_ADDREF_P(pzval);
           return pzval;
       }
       return NULL;
    } else {
       return NULL;
    }
}

static void set_base_home(const char *interface_path)
{
    char *_controllers_home;
    char *_modeles_home;
    char *_views_home;
    spprintf(&_controllers_home, 0, "/modules/%s/controllers/", interface_path);
    spprintf(&_modeles_home, 0, "/modules/%s/models/", interface_path);
    spprintf(&_views_home, 0, "/modules/%s/views/", interface_path);

    zval z_controllers_home;
    zval z_models_home;
    zval z_views_home;
    ZVAL_STRING(&z_controllers_home, _controllers_home);
    ZVAL_STRING(&z_models_home, _modeles_home);
    ZVAL_STRING(&z_views_home, _views_home);
    
    char *m_t="_models_home";
    char *v_t="_views_home";
    char *c_t="_controllers_home";
    zend_string *m = zend_string_init(m_t, strlen(m_t), 0);
    zend_string *v = zend_string_init(v_t, strlen(v_t), 0);
    zend_string *c = zend_string_init(c_t, strlen(c_t), 0);
   
    zval *configs;
    zval *config_instance;
    config_instance = zend_read_static_property(edge_config_ce,  ZEND_STRL("instance"), 1);

    if (Z_TYPE_P(config_instance) == IS_NULL) {
        zval_ptr_dtor(&z_controllers_home);
        zval_ptr_dtor(&z_models_home);
        zval_ptr_dtor(&z_views_home);
        goto end_of_func; 
    }

    configs = zend_read_property(edge_config_ce, config_instance, ZEND_STRL("_data"), 1, NULL);
    zend_hash_update(Z_ARRVAL_P(configs), m, &z_models_home);
    zend_hash_update(Z_ARRVAL_P(configs), v, &z_views_home);
    zend_hash_update(Z_ARRVAL_P(configs), c, &z_controllers_home);

end_of_func:
    efree(_controllers_home);
    efree(_modeles_home);
    efree(_views_home);
    zend_string_release(m);
    zend_string_release(v);
    zend_string_release(c);
}

static void dispatch(zval *obj) 
{
    char *token[1024] = {0};
    zval *path_info;
    path_info = edge_request_query(EDGE_REQUEST_VARS_SERVER, "PATH_INFO");
    if (path_info == NULL) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "[framework error] must specify module like this : http://domain/module");
        return ;
    }
    char *path_info_string;
    char *ptr;
    char *delim = "/";
    char *jt = NULL;
    int offset = 1;
    path_info_string = estrndup(Z_STRVAL_P(path_info), Z_STRLEN_P(path_info));
    if ((jt = php_strtok_r(path_info_string, delim, &ptr)) != NULL) {
        token[0] = jt;
        while ((jt = php_strtok_r(NULL, delim, &ptr)) != NULL) {
            token[offset] = jt;
            offset++;
        }
    }

    if(offset < 3) {
        int j=2;
        while (token[j] == NULL && j >= 0) {
            token[j] = "index";
            j--;
        }
        offset = 3;
    }

    int module_levels = 0;
    char tmp_interface_path[1024] = {0};
    char *interface_path;
    interface_path = tmp_interface_path;
    
    for(;module_levels < offset -2; module_levels++) {
        strcpy(interface_path, token[module_levels]);
        interface_path += strlen(token[module_levels]);
        strcpy(interface_path, "/");
        interface_path +=1;
    }
    
    char *controller;
    char *action;
    interface_path = tmp_interface_path;

    LPS(token[offset-2], controller)
    LPS(token[offset-1], action)

    zval z_controller;
    zval z_action;
    zval z_interface_path;

    ZVAL_STRING(&z_controller, controller);
    ZVAL_STRING(&z_action, action);
    ZVAL_STRING(&z_interface_path, interface_path);

    zval *dispatchInfo;
    dispatchInfo = zend_read_property(edge_router_ce, obj, ZEND_STRL("dispatchInfo"), 1, NULL);
    if (Z_TYPE_P(dispatchInfo) == IS_NULL) {
        array_init(dispatchInfo);
    }
    
    zend_hash_str_update(Z_ARRVAL_P(dispatchInfo), "controller", strlen("controller"), &z_controller);
    zend_hash_str_update(Z_ARRVAL_P(dispatchInfo), "action", strlen("action"), &z_action);
    zend_hash_str_update(Z_ARRVAL_P(dispatchInfo), "interface_path", strlen("interface_path"), &z_interface_path);
    
    set_base_home(interface_path);
    zval_ptr_dtor(path_info);
    efree(controller);
    efree(action);
    efree(path_info_string);
}

zval *get_router_instance(zval *router_instance)
{
    object_init_ex(router_instance, edge_router_ce);
    dispatch(router_instance);
    return router_instance;
}

PHP_METHOD(Edge_Router, __construct)
{
    zval *obj = getThis();
    dispatch(obj);
}

EDGE_STARTUP_FUNCTION(router)
{
    zend_class_entry tmp_ce;
    INIT_CLASS_ENTRY(tmp_ce, "Edge_Router", edge_router_methods);
    edge_router_ce = zend_register_internal_class(&tmp_ce TSRMLS_CC);
    zend_declare_property_null(edge_router_ce, ZEND_STRL("dispatchInfo"), ZEND_ACC_PUBLIC);

    return SUCCESS;
}
