extern zend_class_entry *edge_config_ce; 

typedef struct _edge_config_cache{
    long ctime;
    HashTable *config;
} edge_config_cache;

static int edge_init_modify(zval *filename, long ctime TSRMLS_DC);
static void edge_config_persistent(zval *filename, zval *obj TSRMLS_DC);
static void edge_config_by_persistent(zval *filename, zval *obj TSRMLS_DC);
static void edge_config_copy(HashTable *dst, HashTable *src TSRMLS_DC);

static zval * edge_config_ini_zval_losable(zval *zvalue TSRMLS_DC);
static void edge_config_copy_losable(HashTable *ldst, HashTable *src TSRMLS_DC);

zval *edge_ini_array_copy(zval *zv TSRMLS_DC);
zval *edge_parse_ini_file(zval *zv);
//zval *get_config_instance(zval *obj);
zval *get_config_instance(char *config_path TSRMLS_DC);

PHP_METHOD(Edge_Config, __construct);
PHP_METHOD(Edge_Config, get);
PHP_METHOD(Edge_Config, set);
PHP_METHOD(Edge_Config, reg);
PHP_METHOD(Edge_Config, merge);

EDGE_STARTUP_FUNCTION(config);
