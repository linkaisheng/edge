extern zend_class_entry *edge_config_ce; 

typedef struct _edge_config_cache{
    long ctime;
    HashTable *config;
} edge_config_cache;

static int edge_init_modify(zval *filename, long ctime);
static void edge_config_persistent(zval *filename, zval *obj);

zval edge_parse_ini_file(zval *zval);
zval *get_config_instance(zval *config,const char *config_path);

PHP_METHOD(Edge_Config, __construct);
PHP_METHOD(Edge_Config, get);
PHP_METHOD(Edge_Config, set);
PHP_METHOD(Edge_Config, reg);
PHP_METHOD(Edge_Config, merge);

EDGE_STARTUP_FUNCTION(config);
