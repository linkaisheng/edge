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

int static edge_register_autoload(zval *loader, const char *fname)
{
    zval *z_function;
    zval *arg;
    zval **args[1] = {&arg};
    zval *method;
    zval *ret = NULL;

    MAKE_STD_ZVAL(z_function);
    ZVAL_STRING(z_function, "spl_autoload_register", 0);
    
    MAKE_STD_ZVAL(method);
    ZVAL_STRING(method, fname, 1);

    MAKE_STD_ZVAL(arg);
    array_init(arg);

    zend_hash_next_index_insert(Z_ARRVAL_P(arg), &loader, sizeof(zval *), NULL);
    zend_hash_next_index_insert(Z_ARRVAL_P(arg), &method, sizeof(zval *), NULL);
    
    zend_fcall_info fci = { 
        sizeof(fci),
        EG(function_table),
        z_function,
        NULL,
        &ret,
        1,  
        (zval ***)args,
        NULL,
        1   
    }; 

    if(zend_call_function(&fci, NULL TSRMLS_CC) == FAILURE)
    {
        if(ret)
        {
            zval_ptr_dtor(&ret);
        }
        zval_ptr_dtor(&z_function);   
        zval_ptr_dtor(&arg);
        return 0;
    }
    if(ret)
    {
        zval_ptr_dtor(&ret);
    }
    Z_ADDREF_P(loader);
    efree(z_function);   
    zval_ptr_dtor(&arg);
    /*
    //do some test...
    */
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
    op_array = zend_compile_file(&file_handle, ZEND_INCLUDE TSRMLS_CC);
    if (op_array && file_handle.handle.stream.handle) 
    {
        int dummy = 1;

        if (!file_handle.opened_path)
        {
            file_handle.opened_path = file_path;
        }   

        zend_hash_add(&EG(included_files), file_handle.opened_path, strlen(file_handle.opened_path)+1, (void *)&dummy, sizeof(int), NULL);
    }   
    zend_destroy_file_handle(&file_handle TSRMLS_CC);

    if (op_array)
    {
        zval *result = NULL;
    
        EDGE_BYAF_STORE_EG_ENVIRON();

        EG(return_value_ptr_ptr) = &result;
        EG(active_op_array)      = op_array;
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
        if (!EG(active_symbol_table)) 
        {
            zend_rebuild_symbol_table(TSRMLS_C);
        }
#endif
        zend_execute(op_array TSRMLS_CC);
        destroy_op_array(op_array TSRMLS_CC);
        efree(op_array);
        if (!EG(exception)) 
        {
            if (EG(return_value_ptr_ptr) && *EG(return_value_ptr_ptr)) 
            {
                zval_ptr_dtor(EG(return_value_ptr_ptr));
            }
        }

        EDGE_BYAF_RESTORE_EG_ENVIRON();
        return 1;
    }
    return 0;
}

zval * get_loader_instance()
{
    zval* loader_instance;
    MAKE_STD_ZVAL(loader_instance);
    object_init_ex(loader_instance, edge_loader_ce);
    if(!edge_register_autoload(loader_instance, "autoload"))
    {
        //throw auto load function register error in here..
        printf("autoload function error");
    }

    return loader_instance;
}

PHP_METHOD(Edge_Loader, __construct)
{
    char *lib_path;
    char *lib_prefix;
    int lb_len = 0;
    int lp_len = 0;
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ss", &lib_path, &lb_len, &lib_prefix, &lp_len) == FAILURE)
    {
        return;
    }
    if(lb_len != 0)
    {
        zend_update_property_string(edge_loader_ce, getThis(), LOADER_DEFAULT_LIB_PATH, sizeof(LOADER_DEFAULT_LIB_PATH)-1, lib_path TSRMLS_DC);
    }

    if(lp_len != 0)
    {
        zend_update_property_string(edge_loader_ce, getThis(), LOADER_DEFAULT_LIB_PREFIX, sizeof(LOADER_DEFAULT_LIB_PREFIX)-1, lib_prefix TSRMLS_DC);
    }
    //op spl_autoload_register function.
    if(!edge_register_autoload(getThis(), "autoload"))
    {
        //throw auto load function register error in here..
        printf("autoload function error");
    }

}

PHP_METHOD(Edge_Loader, autoload)
{
    char *class_name;
    char *dir = NULL;
    int cnlen;
    int drlen;
    
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &class_name, &cnlen, &dir, &drlen) == FAILURE)
    {
        printf("autoload get class name error\n");
        return;
    }
    char *_home_path = NULL;
    char *file;
    char *delim = "_";
    
    if(dir == NULL)
    {
        _home_path = estrdup("IfLib/");
    }else
    {
        _home_path = estrdup(dir);
    }

    if(!strncmp(class_name, "If", 2))
    {
        strtok(class_name, delim);
        file = strtok(NULL, delim);
    }else
    {
        class_name[0] = toupper(class_name[0]);
        file = class_name;
    }
    char *include_file_path;
    /*
    include_file_path = emalloc(sizeof(char *) * strlen(EDGE_G(root_path)) + 
                                sizeof(char *) * strlen(_home_path) +
                                sizeof(char *) * strlen(file) + 
                                sizeof(char *) * strlen(".php"));
                                */
    /*
    strcpy(include_file_path, EDGE_G(root_path));
    strcat(include_file_path, _home_path);
    strcat(include_file_path, file);
    strcat(include_file_path, ".php");
    */
    spprintf(&include_file_path, 0, "%s%s%s%s", EDGE_G(root_path), _home_path, file, ".php");
    zval *func_name;
    MAKE_STD_ZVAL(func_name);
    ZVAL_STRING(func_name, "file_exists", 1);

    zval **call_args[1];
    zval *str;
    MAKE_STD_ZVAL(str);
    ZVAL_STRING(str, include_file_path, 1);
    call_args[0] = &str;
    
    zval *ret, *fret;
    MAKE_STD_ZVAL(ret);
    if(call_user_function_ex(CG(function_table), NULL, func_name, &fret, 1, call_args, 0, NULL TSRMLS_CC) == FAILURE)
    {
        ZVAL_FALSE(ret);
        RETURN_ZVAL(ret, 1, 1);
    }
  
    if(!Z_BVAL_P(fret))
    {
        efree(_home_path);
        efree(include_file_path);
        zval_ptr_dtor(&func_name);
        zval_ptr_dtor(&fret);
        zval_ptr_dtor(*call_args);
        ZVAL_FALSE(ret);
        RETURN_ZVAL(ret, 1, 1);
    }

    if(!edge_file_include(include_file_path))
    {
        efree(_home_path);
        efree(include_file_path);
        ZVAL_FALSE(ret);
    }else{
        efree(_home_path);
        efree(include_file_path);
        ZVAL_TRUE(ret);
    }
    zval_ptr_dtor(&func_name);
    zval_ptr_dtor(*call_args);
    zval_ptr_dtor(&fret);
    RETURN_ZVAL(ret, 1, 1);

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
