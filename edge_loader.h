#define LOADER_DEFAULT_LIB_PATH "lib_path"
#define LOADER_DEFAULT_LIB_PREFIX "lib_prefix"

#define EDGE_BYAF_STORE_EG_ENVIRON() \
{ \
    zval ** __old_return_value_pp   = EG(return_value_ptr_ptr); \
    zend_op ** __old_opline_ptr     = EG(opline_ptr); \
    zend_op_array * __old_op_array  = EG(active_op_array);

#define EDGE_BYAF_RESTORE_EG_ENVIRON() \
    EG(return_value_ptr_ptr) = __old_return_value_pp;\
    EG(opline_ptr)           = __old_opline_ptr; \
    EG(active_op_array)      = __old_op_array; \
}

extern zend_class_entry *edge_loader_ce;
EDGE_STARTUP_FUNCTION(loader);

zval *get_loader_instance(zval *loader_instance); 
int static edge_register_autoload(zval *loader, const char *fname);
int edge_file_include(char *file_path);
PHP_METHOD(Edge_Loader, __construct);
PHP_METHOD(Edge_Loader, autoload);
