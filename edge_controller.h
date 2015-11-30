//#define Z_OBJ_P(zval_p) zend_objects_get_address(zval_p TSRMLS_CC)
//#define Z_OBJ_P(zval_p) php_custom_object_fetch_object(Z_OBJ_P(zval_p)) 
//#define Z_CUSTOM_OBJ_P(zv) php_custom_object_fetch_object(Z_OBJ_P(zv));

#define Z_GET_OBJ_P(zv)     \
    zv->value.obj->ce


extern zend_class_entry *edge_controller_ce;
PHP_METHOD(Edge_Controller, __construct);
PHP_METHOD(Edge_Controller, __get);
PHP_METHOD(Edge_Controller, model);
PHP_METHOD(Edge_Controller, get);
PHP_METHOD(Edge_Controller, post);
PHP_METHOD(Edge_Controller, result);
PHP_METHOD(Edge_Controller, check_login);

EDGE_STARTUP_FUNCTION(controller);
