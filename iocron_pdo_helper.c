/*
  +----------------------------------------------------------------------+
  | PHP Version 5 7                                                      |
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "php.h"
#include "iocron_pdo_helper.h"
#include "php_iocron.h"

#ifdef ZTS
#include "TSRM.h"
#endif

void iocron_freeErrorMessage(iocron_pdo_connection *conn TSRMLS_DC) /* {{{ */ {
	if(conn->errorMessage.len > 0) {
		free(conn->errorMessage.message);
		conn->errorMessage.len	= 0;
	}
}
/* }}} */

void iocron_closeConnectionIfExists(iocron_pdo_connection *conn TSRMLS_DC) /* {{{ */ {

	iocron_freeErrorMessage(conn TSRMLS_CC);

#if ZEND_MODULE_API_NO >= 20140815
	if(conn->pdoObject) {
		zval_ptr_dtor_nogc(conn->pdoObject);
	}
#else
	if(conn->isConnectedDatabase == 1) {
		zval_ptr_dtor(&conn->pdoObject);
	}
#endif
	conn->isConnectedDatabase = 0;
}
/* }}} */

short iocron_connectDatabase(iocron_pdo_connection *conn TSRMLS_DC, char *mysql_host_name, char *mysql_port, char *mysql_database_name,
		char *mysql_user_name, char *mysql_password) /* {{{ */ {

	if(conn->isConnectedDatabase == 1)
	{
		return IOCRON_PDO_CONNECTION_SUCCESS;
	}

	zend_class_entry *ce = NULL;

#if ZEND_MODULE_API_NO >= 20140815
	zval classname;
	ZVAL_STRING(&classname, "PDO");
	ce = zend_fetch_class(Z_STR_P(&classname), ZEND_FETCH_CLASS_AUTO TSRMLS_CC);
#else
	ce = zend_fetch_class("PDO", strlen("PDO"), ZEND_FETCH_CLASS_AUTO TSRMLS_CC);
#endif

	if(ce == NULL) {
		conn->errorMessage.message = malloc(512 * sizeof(char));
		sprintf(conn->errorMessage.message, "PDO class not found");
		conn->errorMessage.len = strlen(conn->errorMessage.message);
#if ZEND_MODULE_API_NO >= 20140815
		zval_ptr_dtor(&classname);
#endif
		return IOCRON_PDO_CONNECTION_FAILURE;
	}

#if ZEND_MODULE_API_NO < 20140815
	MAKE_STD_ZVAL(conn->pdoObject);
#endif
	object_init_ex(conn->pdoObject, ce);

	char *dsn = safe_emalloc(1024, sizeof(char), 1);
	sprintf(dsn, "mysql:host=%s;port=%s;dbname=%s", mysql_host_name, mysql_port, mysql_database_name);
	dsn = safe_erealloc(dsn, strlen(dsn), sizeof(char), 1);
	IOCRON_DEBUG(dsn);

#if ZEND_MODULE_API_NO >= 20140815
	zval params[3];
	zval argDSN_ex, argUserName_ex, argPassword_ex, methodName_ex, ret_val_ptr_ex;
	ZVAL_STRING(&argDSN_ex, dsn);
	ZVAL_STRING(&argUserName_ex, IOCRON_G(mysql_user_name));
	ZVAL_STRING(&argPassword_ex, IOCRON_G(mysql_password));
	ZVAL_STRING(&methodName_ex, "__construct");

	params[0] = argDSN_ex;
	params[1] = argUserName_ex;
	params[2] = argPassword_ex;
#else
	zval *ret_val_ptr = NULL;
	zval **params[3];
	zval *argDSN, *argUserName, *argPassword, *methodName;

	MAKE_STD_ZVAL(argDSN);
	MAKE_STD_ZVAL(argUserName);
	MAKE_STD_ZVAL(argPassword);
	MAKE_STD_ZVAL(methodName);
	ZVAL_STRING(argDSN, dsn, 1);
	ZVAL_STRING(argUserName, (char *)mysql_user_name, 1);
	ZVAL_STRING(argPassword, (char *)mysql_password, 1);
	ZVAL_STRING(methodName, "__construct", 1);

	params[0] = &argDSN;
	params[1] = &argUserName;
	params[2] = &argPassword;
#endif

	zend_fcall_info fci = {0};
	zend_fcall_info_cache fci_cache = {0};
	fci.size = sizeof(fci);
	fci.function_table = &ce->function_table;
	fci.symbol_table = NULL;
#if ZEND_MODULE_API_NO >= 20140815
	fci.function_name = methodName_ex;
	fci.object = Z_OBJ_P(conn->pdoObject);
	fci.retval = &ret_val_ptr_ex;
	fci.params = params;
#else
	fci.function_name = methodName;
	fci.object_ptr = (zval *)conn->pdoObject;
	fci.retval_ptr_ptr = &ret_val_ptr;
	fci.params = params;
#endif
	fci.param_count = 3;
	fci.no_separation = 0;

	fci_cache.initialized = 1;
	fci_cache.function_handler = ce->constructor;
	fci_cache.calling_scope = EG(scope);
	fci_cache.called_scope = Z_OBJCE_P(conn->pdoObject);

#if ZEND_MODULE_API_NO >= 20140815
	fci_cache.object = Z_OBJ_P(return_value);
	fci_cache.object = Z_OBJ_P(conn->pdoObject);
	if (zend_call_function(&fci, NULL) == FAILURE) {
#else
	fci_cache.object_ptr = conn->pdoObject;
	if (zend_call_function(&fci, &fci_cache TSRMLS_CC) == FAILURE) {
#endif
		conn->errorMessage.message = malloc(512 * sizeof(char));
		sprintf(conn->errorMessage.message, "Failed initialize PDO");
		conn->errorMessage.len = strlen(conn->errorMessage.message);

		goto IOCRON_FAILURE_CONNECTION;
	}

#if ZEND_MODULE_API_NO >= 20140815
	zval_ptr_dtor(&ret_val_ptr_ex);
	zval_ptr_dtor(&classname);
	zval_ptr_dtor(params);
	zval_ptr_dtor(&argDSN_ex);
	zval_ptr_dtor(&argUserName_ex);
	zval_ptr_dtor(&argPassword_ex);
	zval_ptr_dtor(&methodName_ex);
#else
	if (ret_val_ptr) {
		zval_ptr_dtor(&ret_val_ptr);
	}
	zval_ptr_dtor(&argDSN);
	zval_ptr_dtor(&argUserName);
	zval_ptr_dtor(&argPassword);
	zval_ptr_dtor(&methodName);
#endif

	efree(dsn);

	if (EG(exception))
	{
		conn->errorMessage.message = malloc(512 * sizeof(char));
		sprintf(conn->errorMessage.message, "Failed to connect database");
		conn->errorMessage.len = strlen(conn->errorMessage.message);

		return IOCRON_PDO_CONNECTION_FAILURE;
	}

	conn->isConnectedDatabase = 1;
	return IOCRON_PDO_CONNECTION_SUCCESS;

IOCRON_FAILURE_CONNECTION:
#if ZEND_MODULE_API_NO >= 20140815
	zval_ptr_dtor(&ret_val_ptr_ex);
	zval_ptr_dtor(&classname);
	zval_ptr_dtor(params);
	zval_ptr_dtor(&argDSN_ex);
	zval_ptr_dtor(&argUserName_ex);
	zval_ptr_dtor(&argPassword_ex);
	zval_ptr_dtor(&methodName_ex);
#else
	if (ret_val_ptr) {
		zval_ptr_dtor(&ret_val_ptr);
	}
	zval_ptr_dtor(&argDSN);
	zval_ptr_dtor(&argUserName);
	zval_ptr_dtor(&argPassword);
	zval_ptr_dtor(&methodName);
#endif

	efree(dsn);
	return IOCRON_PDO_CONNECTION_FAILURE;
}
/* }}} */

zval *iocron_executePDOQuery(iocron_pdo_connection *conn, char **query TSRMLS_DC) /* {{{ */ {

	zend_class_entry *ce = NULL;

	#if ZEND_MODULE_API_NO >= 20140815
	zval classname;
	ZVAL_STRING(&classname, "PDO");
	ce = zend_fetch_class(Z_STR_P(&classname), ZEND_FETCH_CLASS_AUTO TSRMLS_CC);
	#else
	ce = zend_fetch_class("PDO", strlen("PDO"), ZEND_FETCH_CLASS_AUTO TSRMLS_CC);
	#endif

	#if ZEND_MODULE_API_NO >= 20140815
	zval ret_val_ptr;
	zval params[1];
	zval argQuery, methodName;
	ZVAL_STRING(&argQuery, *query);
	ZVAL_STRING(&methodName, "query");
	params[0] = argQuery;
	#else
	zval *ret_val_ptr = NULL;
	zval **params[1];
	zval *argQuery, *methodName;
	MAKE_STD_ZVAL(argQuery);
	MAKE_STD_ZVAL(methodName);
	ZVAL_STRING(argQuery, *query, 1);
	ZVAL_STRING(methodName, "query", 1);
	params[0] = &argQuery;
	#endif

	zend_fcall_info fci = {0};
	fci.size = sizeof(fci);
	fci.function_table = &ce->function_table;
	fci.symbol_table = NULL;

	#if ZEND_MODULE_API_NO >= 20140815
	fci.function_name = methodName;
	fci.object = Z_OBJ_P(conn->pdoObject);
	fci.retval = &ret_val_ptr;
	fci.params = params;
	#else
	fci.function_name = methodName;
	fci.object_ptr = conn->pdoObject;
	fci.retval_ptr_ptr = &ret_val_ptr;
	fci.params = params;
	#endif

	fci.param_count = 1;
	fci.no_separation = 0;

#if ZEND_MODULE_API_NO >= 20140815
	if (zend_call_function(&fci, NULL) == FAILURE) {
#else
	if (zend_call_function(&fci, NULL TSRMLS_CC) == FAILURE) {
#endif
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to call PDO:query.");

#if ZEND_MODULE_API_NO >= 20140815
		if (&ret_val_ptr) {
			zval_ptr_dtor(&ret_val_ptr);
		}
		zval_ptr_dtor(&methodName);

		if(&argQuery) {
			zval_ptr_dtor(&argQuery);
		}

		zval_ptr_dtor(&classname);
#else
		if (ret_val_ptr) {
			zval_ptr_dtor(&ret_val_ptr);
		}
		zval_ptr_dtor(&methodName);

		if(argQuery) {
			zval_ptr_dtor(&argQuery);
		}
#endif

		efree(*query);
		return NULL;
	}

#if ZEND_MODULE_API_NO >= 20140815
	zval_ptr_dtor(&methodName);

	if(&argQuery) {
		zval_ptr_dtor(&argQuery);
	}

	zval_ptr_dtor(&classname);
	efree(*query);

	zval *ret_val_ptr_ex = ecalloc(1, sizeof(zval));
	ZVAL_COPY(ret_val_ptr_ex, &ret_val_ptr);
	zval_ptr_dtor(&ret_val_ptr);
#else
	zval_ptr_dtor(&methodName);

	if(argQuery) {
		zval_ptr_dtor(&argQuery);
	}

	efree(*query);
#endif

	if (EG(exception)) {

		if(ret_val_ptr) {
			zval_ptr_dtor(&ret_val_ptr);
		}

		IOCRON_DEBUG("Exception detected!");

		return NULL;
	}else{
		return ret_val_ptr;
	}
}
/* }}} */

zval *iocron_fetchPDOResult(zval *pdoStatement TSRMLS_DC) /* {{{ */ {

	zend_class_entry *statementce = NULL;

#if ZEND_MODULE_API_NO >= 20140815
	zval methodName, fetchType, params[1];
	zval ret_val_ptr;
	ZVAL_LONG(&fetchType, 3);
	ZVAL_STRING(&methodName, "fetchAll");
	statementce = Z_OBJCE_P(pdoStatement TSRMLS_C);
	params[0] = fetchType;
#else
	zval *methodName, *fetchType;
	zval *ret_val_ptr = NULL;
	zval **params[1];
	MAKE_STD_ZVAL(fetchType);
	ZVAL_LONG(fetchType, 3);
	MAKE_STD_ZVAL(methodName);
	ZVAL_STRING(methodName, "fetchAll", 1);
	statementce = zend_get_class_entry(pdoStatement TSRMLS_CC);
	params[0] = &fetchType;
#endif


	zend_fcall_info fcifetch = {0};
	fcifetch.size = sizeof(fcifetch);
	fcifetch.function_table = &statementce->function_table;
#if ZEND_MODULE_API_NO >= 20140815
	fcifetch.object = Z_OBJ_P(pdoStatement);
	fcifetch.retval = &ret_val_ptr;
#else
	fcifetch.object_ptr = pdoStatement;
	fcifetch.retval_ptr_ptr = &ret_val_ptr;
#endif
	fcifetch.params = params;
	fcifetch.function_name = methodName;
	fcifetch.symbol_table = NULL;
	fcifetch.param_count = 1;
	fcifetch.no_separation = 1;

#if ZEND_MODULE_API_NO >= 20140815
	if (zend_call_function(&fcifetch, NULL) == FAILURE) {
#else
	if (zend_call_function(&fcifetch, NULL TSRMLS_CC) == FAILURE) {
#endif
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to call PDOStatement:fetch");

#if ZEND_MODULE_API_NO >= 20140815
		if (&ret_val_ptr) {
			zval_ptr_dtor(&ret_val_ptr);
		}

		zval_ptr_dtor(&fetchType);
		zval_ptr_dtor(&methodName);
		zval_ptr_dtor(pdoStatement);
		efree(pdoStatement);
#else
		if (ret_val_ptr) {
			zval_ptr_dtor(&ret_val_ptr);
		}

		if(fetchType) {
			zval_ptr_dtor(&fetchType);
		}

		zval_ptr_dtor(&methodName);
		zval_ptr_dtor(&pdoStatement);
#endif
	}

#if ZEND_MODULE_API_NO >= 20140815
	zval_ptr_dtor(&fetchType);
	zval_ptr_dtor(&methodName);
	zval_ptr_dtor(params);
	zval_ptr_dtor(pdoStatement);
	efree(pdoStatement);
	zval *ret_val_ptr_ex = ecalloc(1, sizeof(zval));
	ZVAL_COPY(ret_val_ptr_ex, &ret_val_ptr);
	zval_ptr_dtor(&ret_val_ptr);
	return ret_val_ptr_ex;
#else
	if(fetchType) {
		zval_ptr_dtor(&fetchType);
	}
	zval_ptr_dtor(&methodName);
	zval_ptr_dtor(&pdoStatement);
	return ret_val_ptr;
#endif

}
/* }}} */

void iocron_closePDOCursor(zval *pdoStatement TSRMLS_DC) /* {{{ */ {

	zend_class_entry *statementce = NULL;

#if ZEND_MODULE_API_NO >= 20140815
	zval methodName;
	zval ret_val_ptr;
	ZVAL_STRING(&methodName, "closeCursor");
	statementce = Z_OBJCE_P(pdoStatement TSRMLS_C);
#else
	zval *methodName;
	zval *ret_val_ptr = NULL;
	MAKE_STD_ZVAL(methodName);
	ZVAL_STRING(methodName, "closeCursor", 1);
	statementce = zend_get_class_entry(pdoStatement TSRMLS_CC);
#endif

	zend_fcall_info fciclose = {0};
	fciclose.size = sizeof(fciclose);
	fciclose.function_table = &statementce->function_table;
#if ZEND_MODULE_API_NO >= 20140815
	fciclose.object = Z_OBJ_P(pdoStatement);
	fciclose.retval = &ret_val_ptr;
#else
	fciclose.object_ptr = pdoStatement;
	fciclose.retval_ptr_ptr = &ret_val_ptr;
#endif
	fciclose.params = NULL;
	fciclose.function_name = methodName;
	fciclose.symbol_table = NULL;
	fciclose.param_count = 0;
	fciclose.no_separation = 1;

#if ZEND_MODULE_API_NO >= 20140815
	if (zend_call_function(&fciclose, NULL) == FAILURE) {
#else
	if (zend_call_function(&fciclose, NULL TSRMLS_CC) == FAILURE) {
#endif
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to call PDOStatement:closeCursor");

#if ZEND_MODULE_API_NO >= 20140815
		if (&ret_val_ptr) {
			zval_ptr_dtor(&ret_val_ptr);
		}

		zval_ptr_dtor(&methodName);
		zval_ptr_dtor(pdoStatement);
		efree(pdoStatement);
#else
		if (ret_val_ptr) {
			zval_ptr_dtor(&ret_val_ptr);
		}

		zval_ptr_dtor(&methodName);
		zval_ptr_dtor(&pdoStatement);
#endif
	}

#if ZEND_MODULE_API_NO >= 20140815
	zval_ptr_dtor(&methodName);
	zval_ptr_dtor(pdoStatement);
	efree(pdoStatement);
	zval_ptr_dtor(&ret_val_ptr);
#else
	zval_ptr_dtor(&methodName);
	zval_ptr_dtor(&pdoStatement);
	zval_ptr_dtor(&ret_val_ptr);
#endif
}
/* }}} */

zval *iocron_getPDOLastInsertId(iocron_pdo_connection *conn TSRMLS_DC) /* {{{ */ {

	zend_class_entry *ce = NULL;

	#if ZEND_MODULE_API_NO >= 20140815
	zval classname;
	ZVAL_STRING(&classname, "PDO");
	ce = zend_fetch_class(Z_STR_P(&classname), ZEND_FETCH_CLASS_AUTO TSRMLS_CC);
	#else
	ce = zend_fetch_class("PDO", strlen("PDO"), ZEND_FETCH_CLASS_AUTO TSRMLS_CC);
	#endif

#if ZEND_MODULE_API_NO >= 20140815
	zval ret_val_ptr;
	zval methodName;
	ZVAL_STRING(&methodName, "lastInsertId");
#else
	zval *ret_val_ptr = NULL;
	zval *methodName;
	MAKE_STD_ZVAL(methodName);
	ZVAL_STRING(methodName, "lastInsertId", 1);
#endif

	zend_fcall_info fci = {0};
	fci.size = sizeof(fci);
	fci.function_table = &ce->function_table;
	fci.symbol_table = NULL;
#if ZEND_MODULE_API_NO >= 20140815
	fci.function_name = methodName;
	fci.object = Z_OBJ_P(conn->pdoObject);
	fci.retval = &ret_val_ptr;
	fci.params = NULL;
#else
	fci.function_name = methodName;
	fci.object_ptr = conn->pdoObject;
	fci.retval_ptr_ptr = &ret_val_ptr;
	fci.params = NULL;
#endif
	fci.param_count = 0;
	fci.no_separation = 0;

#if ZEND_MODULE_API_NO >= 20140815
	if (zend_call_function(&fci, NULL) == FAILURE) {
#else
	if (zend_call_function(&fci, NULL TSRMLS_CC) == FAILURE) {
#endif
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to call PDO:query.");

#if ZEND_MODULE_API_NO >= 20140815
		if (&ret_val_ptr) {
			zval_ptr_dtor(&ret_val_ptr);
		}
		zval_ptr_dtor(&classname);
#else
		if (ret_val_ptr) {
			zval_ptr_dtor(&ret_val_ptr);
		}
#endif

		zval_ptr_dtor(&methodName);
		return NULL;
	}

#if ZEND_MODULE_API_NO >= 20140815
	zval_ptr_dtor(&classname);
#endif

	zval_ptr_dtor(&methodName);

	return ret_val_ptr;
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
