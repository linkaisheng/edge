#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
//#include "Zend/zend_interfaces.h"

#include "php_edge.h"
#include "edge_router.h"
#include "edge_request.h"


zend_class_entry *edge_request_ce;

zend_function_entry edge_request_methods[] = {
    PHP_ME(Edge_Request, get, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Edge_Request, post, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Edge_Request, request, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Edge_Request, cookie, NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

zval * get_request_instance()
{
    zval *request_instance;
    MAKE_STD_ZVAL(request_instance);
    object_init_ex(request_instance, edge_request_ce);
    return request_instance;
}

PHP_METHOD(Edge_Request, get)
{
    zval *name;
    long len;
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE)
    {
        return;   
    }

    zval *data;
    data = edge_request_query(EDGE_REQUEST_VARS_GET, Z_STRVAL_P(name));
    RETURN_ZVAL(data, 1, 1); 
}

PHP_METHOD(Edge_Request, post)
{
    zval *name;
    long len;
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len)== FAILURE)
    {
        return;
    }
    zval *data;
    data = edge_request_query(EDGE_REQUEST_VARS_POST, Z_STRVAL_P(data));
    RETURN_ZVAL(data, 1, 1);
}

PHP_METHOD(Edge_Request, request)
{

}

PHP_METHOD(Edge_Request, cookie)
{

}

EDGE_STARTUP_FUNCTION(request)
{
    zend_class_entry tmp_ce;
    INIT_CLASS_ENTRY(tmp_ce, "Edge_Request", edge_request_methods);
    edge_request_ce = zend_register_internal_class(&tmp_ce TSRMLS_CC);
    return SUCCESS;
}
