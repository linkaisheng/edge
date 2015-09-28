
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_edge.h"

#include "edge_config.h"


ZEND_DECLARE_MODULE_GLOBALS(edge)

/* True global resources - no need for thread safety here */
static int le_edge;

/* {{{ edge_functions[]
 *
 * Every user visible function must have an entry in edge_functions[].
 */
const zend_function_entry edge_functions[] = {
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ edge_module_entry
 */
zend_module_entry edge_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"edge",
	edge_functions,
	PHP_MINIT(edge),
	PHP_MSHUTDOWN(edge),
	PHP_RINIT(edge),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(edge),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(edge),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_EDGE
ZEND_GET_MODULE(edge)
#endif

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(edge)
{
	EDGE_STARTUP(config);
    EDGE_STARTUP(router);
    EDGE_STARTUP(request);
    EDGE_STARTUP(loader);
    EDGE_STARTUP(controller);
    EDGE_STARTUP(core);
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(edge)
{
    if (EDGE_G(configs)) {
        zend_hash_destroy(EDGE_G(configs));
        pefree(EDGE_G(configs), 1); 
    }   

    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
*/
PHP_RINIT_FUNCTION(edge)
{
    EDGE_G(root_path) = NULL;
    EDGE_G(config_path) = NULL;
    MAKE_STD_ZVAL(EDGE_G(regs));
    array_init(EDGE_G(regs));
    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
*/
PHP_RSHUTDOWN_FUNCTION(edge)
{
    if (EDGE_G(root_path)){
        efree(EDGE_G(root_path));
    }
    
    if (EDGE_G(config_path)){
        efree(EDGE_G(config_path));
    }

    zval_ptr_dtor(&EDGE_G(regs));
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
*/
PHP_MINFO_FUNCTION(edge)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "edge support", "enabled");
    php_info_print_table_end();

    /* Remove comments if you have entries in php.ini
       DISPLAY_INI_ENTRIES();
       */
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
