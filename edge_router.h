/*
 *   +----------------------------------------------------------------------+
 *     | Edge Framework                                                |
 *   +----------------------------------------------------------------------+
 *   +----------------------------------------------------------------------+
 *     | Author: linkaisheng  <carlsonlin@tencent.com>                 |
 *   +----------------------------------------------------------------------+
 */

#define EDGE_REQUEST_VARS_GET           TRACK_VARS_GET 
#define EDGE_REQUEST_VARS_POST          TRACK_VARS_POST
#define EDGE_REQUEST_vARS_REQUEST       TRACK_VARS_REQUEST
#define EDGE_REQUEST_VARS_COOKIE        TRACK_VARS_COOKIE
#define EDGE_REQUEST_VARS_SERVER        TRACK_VARS_SERVER
#define EDGE_REQUEST_VARS_ENV           TRACK_VARS_ENV

#define EDGE_REQUEST_BASEMODULE "base_module"
#define EDGE_REQUEST_MODULE "module"
#define EDGE_REQUEST_CONTROLLER "controller"
#define EDGE_REQUEST_ACTION "action"

#define EDGE_MODLE_HOME "_models_home"
#define EDGE_CONTROLLER_HOME "_controllers_home"
#define EDGE_VIEW_HOME "_views_home"

#define LPS(token, op)                  \
    if(token == NULL){                  \
        spprintf(&op, 0, "%s", "index");  \
    }else{                              \
        spprintf(&op, 0, "%s", token);  \
    }                                   \
    

    


extern zend_class_entry *edge_router_ce;

PHP_METHOD(Edge_Router, __construct);
zval * edge_request_query(int type, char *name);
zval * get_router_instance(zval *router_instance);
static void set_base_home(const char *interface_path);
static void dispatch(zval *obj);
static void init_ce(zval *obj);

EDGE_STARTUP_FUNCTION(router);
