extern zend_class_entry *edge_core_ce;

EDGE_STARTUP_FUNCTION(core);
PHP_METHOD(Edge_Core, __construct);
PHP_METHOD(Edge_Core, bootstrap);
PHP_METHOD(Edge_Core, reg);
static void set_root_path(char *path);
