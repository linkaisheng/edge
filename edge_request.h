extern zend_class_entry *edge_request_ce;
EDGE_STARTUP_FUNCTION(request);

PHP_METHOD(Edge_Request, get);
PHP_METHOD(Edge_Request, post);
PHP_METHOD(Edge_Request, request);
PHP_METHOD(Edge_Request, cookie);
zval *get_request_instance();
