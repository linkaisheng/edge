#define Z_OBJ_P(zval_p) zend_objects_get_address(zval_p TSRMLS_CC)

extern zend_class_entry *edge_controller_ce;
char * edge_std_object_get_class_name(const zval *object, int parent TSRMLS_DC);
PHP_METHOD(Edge_Controller, __construct);
PHP_METHOD(Edge_Controller, __get);
PHP_METHOD(Edge_Controller, model);
PHP_METHOD(Edge_Controller, get);
PHP_METHOD(Edge_Controller, post);
PHP_METHOD(Edge_Controller, result);
PHP_METHOD(Edge_Controller, check_login);

EDGE_STARTUP_FUNCTION(controller);
