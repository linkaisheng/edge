
/* $Id$ */

#ifndef PHP_EDGE_H
#define PHP_EDGE_H

extern zend_module_entry edge_module_entry;
#define phpext_edge_ptr &edge_module_entry

#ifdef PHP_WIN32
#	define PHP_EDGE_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_EDGE_API __attribute__ ((visibility("default")))
#else
#	define PHP_EDGE_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#ifdef ZTS
#define EDGE_G(v) TSRMG(edge_globals_id, zend_edge_globals *, v)
#else
#define EDGE_G(v) (edge_globals.v)
#endif

#define EDGE_STARTUP_FUNCTION(module)       ZEND_MINIT_FUNCTION(edge_##module)
#define EDGE_STARTUP(module)                ZEND_MODULE_STARTUP_N(edge_##module)(INIT_FUNC_ARGS_PASSTHRU)

ZEND_BEGIN_MODULE_GLOBALS(edge)
    HashTable *configs;
    zval *cacheData;
    zval regs;
    char *root_path;
    char *config_path;
ZEND_END_MODULE_GLOBALS(edge)

PHP_MINIT_FUNCTION(edge);
PHP_MSHUTDOWN_FUNCTION(edge);
PHP_RINIT_FUNCTION(edge);
PHP_RSHUTDOWN_FUNCTION(edge);
PHP_MINFO_FUNCTION(edge);

extern ZEND_DECLARE_MODULE_GLOBALS(edge);





#endif	/* PHP_EDGE_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
