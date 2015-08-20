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
    zval *config;
    zval *config_data;
    zval **ppval;
    config = zend_read_static_property(edge_core_ce, ZEND_STRL("config"), 1 TSRMLS_DC);
    config_data = zend_read_property(edge_config_ce, config, ZEND_STRL("_data"), 1 TSRMLS_DC);
    
    if(zend_hash_find(Z_ARRVAL_P(config_data), "_models_home", strlen("_models_home")+1, (void **)&ppval) == FAILURE)
    {
        RETURN_FALSE;
    }
    PHPWRITE(Z_STRVAL_PP(ppval), Z_STRLEN_PP(ppval));
    zend_declare_property_string(edge_controller_ce, ZEND_STRL("_models_home"), Z_STRVAL_PP(ppval), ZEND_ACC_PUBLIC TSRMLS_DC);
}

PHP_METHOD(Edge_Controller, get)
{
    char *name;
    int nlen = 0;
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &nlen) == FAILURE)
    {
        return;
    }
    zval * data;
    data = edge_request_query(EDGE_REQUEST_VARS_GET, name);
    if(Z_TYPE_P(data) == IS_NULL && nlen != 0)
    {
        if(strncmp("model", name, nlen) == 0){
            zval *arg;
            zval *ret;

            zend_object *zobj;
            zend_class_entry *ce; 
            zobj = Z_OBJ_P(getThis());
            ce = zobj->ce;

            char *controller_prefix;
            controller_prefix = estrndup(ce->name, ce->name_length - strlen("Controller"));
            char *model_class_name;
            spprintf(&model_class_name, 0, "%s", controller_prefix);
            MAKE_STD_ZVAL(arg);
            ZVAL_STRING(arg, model_class_name, 1);
            zend_call_method_with_1_params(&getThis(), edge_controller_ce, NULL, "model", &ret, arg);
            efree(model_class_name);
            zval_ptr_dtor(&arg);
            
            if(!Z_BVAL_P(ret)){
                RETURN_ZVAL(ret, 1, 1);
            }

            zend_class_entry *model_ce;
            model_ce = Z_OBJCE_P(ret);
              
            zval **cfptr;
            if(zend_hash_find(&((model_ce)->function_table), "__construct", strlen("__construct")+1, (void **)&cfptr) == SUCCESS)
            {
                 zval *cretval;
                 zend_call_method(&ret, model_ce, NULL, "__construct", strlen("__construct"), &cretval, 0, NULL, NULL TSRMLS_CC);
                 zval_ptr_dtor(&cretval);
            }
            
            efree(controller_prefix);
            zval_ptr_dtor(&data);
            RETURN_ZVAL(ret, 1, 1);
        }else if(strncmp("response", name, nlen) == 0){

        }else if(strncmp("login", name, nlen) == 0){
        
        }

    }
    RETURN_ZVAL(data, 1, 1);
}

PHP_METHOD(Edge_Controller, check_login)
{

    zval *chktype, *uin, *key;
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zzz", &chktype, &uin, &key) == FAILURE)
    {
        RETURN_FALSE;
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
    //php_var_dump(&uin, 1 TSRMLS_CC); 

    zval *fret;
    MAKE_STD_ZVAL(fret);
    ZVAL_STRING(fret, Z_STRVAL_P(uin), 1);
    //php_var_dump(&fret, 1 TSRMLS_CC);

    array_init(return_value);
    add_assoc_string(return_value, "nickname", Z_STRVAL_P(fret), 1);
    zval_ptr_dtor(&fret);
    //RETURN_ZVAL(fret, 1, 1);
    /*

    char *tuin;
    array_init(return_value);
    add_assoc_long(return_value, "uin", 610917472);
    */


}


PHP_METHOD(Edge_Controller, post)
{
    char *name;
    int nlen;
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &nlen) == FAILURE)
    {
        return;
    }
    zval * data;
    data = edge_request_query(EDGE_REQUEST_VARS_POST, name);
    RETURN_ZVAL(data, 1, 1);
}

PHP_METHOD(Edge_Controller, model)
{
    char *model_name = NULL;
    char *model_dir = NULL;
    int mnlen=0;
    int mdlen=0;
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &model_name, &mnlen, &model_dir, &mdlen) == FAILURE)
    {
        RETURN_FALSE;
    }
    
    char *model_class_name;
    int class_name_len;
    class_name_len = spprintf(&model_class_name, 0, "%s%s", model_name, "Model");

    zval **z_obj;
    if(zend_hash_find(Z_ARRVAL_P(EDGE_G(regs)), model_class_name, class_name_len+1, (void **)&z_obj) == SUCCESS)
    {
        efree(model_class_name);
        RETURN_ZVAL(*z_obj, 1, 0);
    }
    
    zval *config;
    zval *config_data;
    config = zend_read_static_property(edge_core_ce, ZEND_STRL("config"), 1 TSRMLS_DC);
    config_data = zend_read_property(edge_config_ce, config, ZEND_STRL("_data"), 1 TSRMLS_DC);

    zval **models_home_pp;
    if(zend_hash_find(Z_ARRVAL_P(config_data), "_models_home", strlen("_models_home")+1, (void **)&models_home_pp) == FAILURE)
    {
        RETURN_FALSE;
    }
    
    zval *z_model_name;
    MAKE_STD_ZVAL(z_model_name);
    ZVAL_STRING(z_model_name, model_class_name, 1);

    zval *loader;
    zval *ret;
    loader = zend_read_static_property(edge_core_ce, ZEND_STRL("loader"), 1 TSRMLS_DC);
    zend_call_method_with_2_params(&loader, Z_OBJCE_P(loader), NULL, "autoload", &ret, z_model_name, *models_home_pp);
    zval_ptr_dtor(&z_model_name);
    if(!Z_BVAL_P(ret))
    {
        char *errorMsg;
        int errorMsgLen;
        errorMsgLen = spprintf(&errorMsg, 0, "[ERROR]model %s%s load error\n", Z_STRVAL_PP(models_home_pp), model_class_name);
        PHPWRITE(errorMsg, errorMsgLen);
        zval_ptr_dtor(&ret);
        efree(model_class_name);
        efree(errorMsg);
        RETURN_FALSE;
    }
    zval_ptr_dtor(&ret);

    zend_class_entry **model_ce;
    if(zend_lookup_class(model_class_name, class_name_len, &model_ce TSRMLS_CC) == FAILURE)
    {
        char *errorMsg;
        int errorMsgLen;
        errorMsgLen = spprintf(&errorMsg, 0, "[ERROR]Model class %s does not exist\n", model_class_name);
        PHPWRITE(errorMsg, errorMsgLen);
        efree(model_class_name);
        RETURN_FALSE;
    }

    zval *model_obj;
    MAKE_STD_ZVAL(model_obj);
    object_init_ex(model_obj, *model_ce);
    zend_hash_update(Z_ARRVAL_P(EDGE_G(regs)), model_class_name, class_name_len+1, (void **)&model_obj, sizeof(zval *), NULL);

    efree(model_class_name);
    RETURN_ZVAL(model_obj, 1, 0);
}

PHP_METHOD(Edge_Controller, result)
{
    zval *data;
    char *msg;
    int mlen;
    long code;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|as", &code, &data, &msg, &mlen) == FAILURE)
    {
        RETURN_FALSE;
    }

    Z_ADDREF_P(data);
    array_init(return_value);  
    add_assoc_long(return_value, "code", code);
    add_assoc_string(return_value, "msg", msg, 1);
    add_assoc_zval(return_value, "data", data);
}
EDGE_STARTUP_FUNCTION(controller)
{
    zend_class_entry tmp_ce;
    INIT_CLASS_ENTRY(tmp_ce, "Edge_Controller", edge_controller_methods);
    edge_controller_ce = zend_register_internal_class(&tmp_ce TSRMLS_CC);

    //zend_declare_property_null(edge_controller_ce, ZEND_STRL("model"), ZEND_ACC_PUBLIC TSRMLS_DC);
    return SUCCESS; 
}
