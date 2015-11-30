#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_edge.h"
#include "edge_config.h"
#include "edge_loader.h"

zend_class_entry *edge_loader_ce;

zend_function_entry edge_loader_methods[] = {
    PHP_ME(Edge_Loader, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Edge_Loader, autoload, NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

int static edge_register_autoload(zval *loader_instance, const char *func_name)
{
    
    zval internal_function, loader_method, autoload, ret;
    zval params[1];
    
    array_init(&autoload);
    
    ZVAL_STRING(&loader_method, func_name);

    zend_hash_next_index_insert(Z_ARRVAL(autoload), loader_instance);
    zend_hash_next_index_insert(Z_ARRVAL(autoload), &loader_method);
  
    ZVAL_COPY(&params[0], &autoload);
    ZVAL_STRING(&internal_function, "spl_autoload_register");
   
    zend_fcall_info fci = {sizeof(fci), EG(function_table), internal_function, NULL, &ret, params, NULL, 1, 1};
    
    if (zend_call_function(&fci, NULL) == FAILURE) {
        if (!Z_ISUNDEF(ret)) {
            zval_ptr_dtor(&ret);
        }
        zval_ptr_dtor(&internal_function);
        zval_ptr_dtor(&autoload);
        zval_ptr_dtor(&params[0]);
        php_error_docref(NULL, E_WARNING, "[EDGE_WARNING]Unable to register autoload function %s", func_name);
        return 0;
    }

    if (!Z_ISUNDEF(ret)) {
        zval_ptr_dtor(&ret);
    }
    Z_ADDREF_P(loader_instance);
    zval_ptr_dtor(&internal_function);
    zval_ptr_dtor(&autoload);
    zval_ptr_dtor(&params[0]);
    
    return 1;
}

int  edge_file_include(char *file_path)
{
    //include file 操作，没有直接的zend_api函数，如果使用，则需要在php执行编译的opcode中操作
    //以下的代码借鉴了yaf框架里面的include逻辑
    zend_file_handle file_handle;
    zend_op_array   *op_array;
    file_handle.filename = file_path;
    file_handle.free_filename = 0;
    file_handle.type = ZEND_HANDLE_FILENAME;
    file_handle.opened_path = NULL;
    file_handle.handle.fp = NULL;
    op_array = zend_compile_file(&file_handle, ZEND_INCLUDE);
    if (op_array && file_handle.handle.stream.handle) {
        //int dummy = 1;
        zval dummy;
        ZVAL_NULL(&dummy);
        if (!file_handle.opened_path) {
            //file_handle.opened_path = file_path;
            file_handle.opened_path =  zend_string_init(file_path, strlen(file_path), 0);;
        }   

        //zend_hash_add(&EG(included_files), file_handle.opened_path, strlen(file_handle.opened_path)+1, (void *)&dummy, sizeof(int), NULL);
        zend_hash_add(&EG(included_files), file_handle.opened_path, &dummy);
    }   
    zend_destroy_file_handle(&file_handle);

    if (op_array) {
        zval result;
        ZVAL_UNDEF(&result);
        
        zend_execute(op_array, &result);
        destroy_op_array(op_array);
        efree(op_array);

        if (!EG(exception)) {
            zval_ptr_dtor(&result);
        }

        return 1;
    }
    return 0;
}

zval * get_loader_instance(zval *loader_instance)
{
    object_init_ex(loader_instance, edge_loader_ce);
    if (!edge_register_autoload(loader_instance, "autoload")) {
        php_error_docref(NULL, E_WARNING, "[EDGE_WARNING]Unable to rebuid the symbol table");
    }
    return loader_instance;
}

PHP_METHOD(Edge_Loader, __construct)
{
    //char *lib_path;
    //char *lib_prefix;
    zend_string *lib_path = NULL;
    zend_string *lib_prefix = NULL;
    if(zend_parse_parameters(ZEND_NUM_ARGS(), "|SS", &lib_path, &lib_prefix) == FAILURE) {
        return;
    }
    if (lib_path != NULL) {
        //zend_update_property_string(edge_loader_ce, getThis(), LOADER_DEFAULT_LIB_PATH, sizeof(LOADER_DEFAULT_LIB_PATH)-1, lib_path TSRMLS_DC);
        zend_update_property_string(edge_loader_ce, getThis(), ZEND_STRL(LOADER_DEFAULT_LIB_PATH), ZSTR_VAL(lib_path));
    }
    if (lib_prefix != NULL) {
        //zend_update_property_string(edge_loader_ce, getThis(), LOADER_DEFAULT_LIB_PREFIX, sizeof(LOADER_DEFAULT_LIB_PREFIX)-1, lib_prefix TSRMLS_DC);
        zend_update_property_string(edge_loader_ce, getThis(), ZEND_STRL(LOADER_DEFAULT_LIB_PREFIX), ZSTR_VAL(lib_prefix));
    }
    
    if (!edge_register_autoload(getThis(), "autoload")) {
        //throw auto load function register error in here..
        php_error_docref(NULL, E_WARNING, "[EDGE_WARNING]Unable to rebuid the symbol table");
    }

}

PHP_METHOD(Edge_Loader, autoload)
{
    zend_string *class_name = NULL;
    zend_string *dir = NULL;
    int include_ret_flag = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|S", &class_name, &dir) == FAILURE) {
        php_error_docref(NULL, E_WARNING, "Unable to get class name");
        return;
    }
    char *class_name_str = ZSTR_VAL(class_name);

    char *_home_path = NULL;
    char *file;
    char *delim = "_";
   
    if (dir == NULL) {
        zend_string *lib_path = zend_string_init("LIB_PATH", sizeof("LIB_PATH")-1, 0);
        zval *c;
        c = zend_get_constant_ex(lib_path, NULL, ZEND_FETCH_CLASS_SILENT);
        if (c) {
            _home_path = estrdup(Z_STRVAL_P(c));
            zval_dtor(c);
        } else {
            _home_path = estrdup("IfLib/");
        }
        zend_string_release(lib_path);
    } else {
        _home_path = estrdup(ZSTR_VAL(dir));
    }

    if (!strncmp(class_name_str, "If", 2) || !strncmp(class_name_str, "IF", 2)) {
        strtok(class_name_str, delim);
        file = strtok(NULL, delim);
    } else {
        file = class_name_str;
    }
    file[0] = toupper(file[0]);
    char *include_file_path;
    spprintf(&include_file_path, 0, "%s%s%s%s", EDGE_G(root_path), _home_path, file, ".php");
    
    zval func_name;
    ZVAL_STRING(&func_name, "file_exists");

    zval call_args[1];
    zval str;
    ZVAL_STRING(&str, include_file_path);
    call_args[0] = str;
    
    zval fret;
    if (call_user_function_ex(CG(function_table), NULL, &func_name, &fret, 1, call_args, 0, NULL) == FAILURE) {
        php_error_docref(NULL, E_WARNING, "[EDGE_WARNING] Unable call internal function 'file_exists'");
        goto finally_operation;
    }
    
    if (Z_TYPE(fret) == IS_FALSE) {
        //php_error_docref(NULL, E_WARNING, "[EDGE_WARNING] Unable load %s, this file does not exist!", include_file_path);
        goto finally_operation;
    }
   
    include_ret_flag = edge_file_include(include_file_path);
    
finally_operation:
    efree(include_file_path);
    efree(_home_path);
    zval_ptr_dtor(&func_name);
    zval_ptr_dtor(&str);
    if (!Z_ISUNDEF(fret)) {
        zval_ptr_dtor(&fret);
    }
    if (include_ret_flag) {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}

EDGE_STARTUP_FUNCTION(loader)
{
    zend_class_entry tmp_ce;
    INIT_CLASS_ENTRY(tmp_ce, "Edge_Loader", edge_loader_methods);
    edge_loader_ce = zend_register_internal_class(&tmp_ce TSRMLS_CC);
    zend_declare_property_string(edge_loader_ce, ZEND_STRL(LOADER_DEFAULT_LIB_PATH), "IfLib", ZEND_ACC_PUBLIC TSRMLS_DC);
    zend_declare_property_string(edge_loader_ce, ZEND_STRL(LOADER_DEFAULT_LIB_PREFIX), "If", ZEND_ACC_PUBLIC TSRMLS_DC);
    return SUCCESS;
}
