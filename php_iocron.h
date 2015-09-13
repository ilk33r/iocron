/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: ilker özcan <iletisim@ilkerozcan.com.tr>                     |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_IOCRON_H
#define PHP_IOCRON_H

extern zend_module_entry iocron_module_entry;
#define phpext_iocron_ptr &iocron_module_entry

/*
#define PHP_IOCRON_VERSION "0.1.0" Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_IOCRON_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_IOCRON_API __attribute__ ((visibility("default")))
#else
#	define PHP_IOCRON_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_php_iocron_version.h"
#include "php_php_iocron_debug.h"

#ifndef PHP_IOCRON_VERSION
#	define PHP_IOCRON_VERSION "~ Dev"
#endif

#ifndef PHP_IOCRON_DEBUG
#	define PHP_IOCRON_DEBUG 0
#endif

#include "iocron_pdo_helper.h"

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:  */

ZEND_BEGIN_MODULE_GLOBALS(iocron)
	char *mysql_database_name;
	char *mysql_password;
	char *mysql_user_name;
	char *mysql_port;
	char *mysql_host_name;
ZEND_END_MODULE_GLOBALS(iocron)

/* In every utility function you add that needs to use variables 
   in php_iocron_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as IOCRON_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#	if ZEND_MODULE_API_NO >= 20140815
#		define IOCRON_G(v) ZEND_TSRMG(iocron_globals_id, zend_iocron_globals *, v)
#	else
#		define IOCRON_G(v) TSRMG(iocron_globals_id, zend_iocron_globals *, v)
#	endif
#else
#	define IOCRON_G(v) (iocron_globals.v)
#endif

/*
	extern zend_module_entry serverapi_module_entry;
	#define phpext_serverapi_ptr &serverapi_module_entry
*/
typedef struct _iocron_objects {
	zend_object obj;

	char *objectId;
	int objectIdLen;
	long cron_group_id;
	long cron_job_id;

	iocron_pdo_connection *dbConnection;
} iocron_objects;

extern zend_module_entry iocron_module_entry;

#if ZEND_MODULE_API_NO >= 20140815
static inline iocron_objects *php_iocron_object(zend_object *inObj) {
	return (iocron_objects *)((char*)(inObj) - XtOffsetOf(iocron_objects, obj));
}

#	define IOCRON_FETCH_OBJECTS(zobj) php_iocron_object(Z_OBJ_P((zobj)))
#else
#	define IOCRON_FETCH_OBJECTS(zobj) (iocron_objects*)zend_objects_get_address((zobj) TSRMLS_CC)
#endif

#if PHP_IOCRON_DEBUG == 1
#	define IOCRON_DEBUG(m) {                                                              \
							{                                                             \
								fprintf(stderr, "\n ---IOCron DEBUG: %s! ---\n", m);      \
							}                                                             \
						}
#else
#	define IOCRON_DEBUG(m) {}
#endif

#endif	/* PHP_IOCRON_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
