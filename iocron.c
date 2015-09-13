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

#include <unistd.h>
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_iocron.h"
#include "ext/standard/php_string.h"

/* If you declare any globals in php_iocron.h uncomment this:
*/
ZEND_DECLARE_MODULE_GLOBALS(iocron)

/* True global resources - no need for thread safety here */
static int le_iocron;

static zend_class_entry *iocron_ce;
static zend_object_handlers iocron_handlers;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
*/
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("iocron.mysql_database_name", "", PHP_INI_ALL, OnUpdateString, mysql_database_name, zend_iocron_globals, iocron_globals)
	STD_PHP_INI_ENTRY("iocron.mysql_password", "", PHP_INI_ALL, OnUpdateString, mysql_password, zend_iocron_globals, iocron_globals)
	STD_PHP_INI_ENTRY("iocron.mysql_user_name", "", PHP_INI_ALL, OnUpdateString, mysql_user_name, zend_iocron_globals, iocron_globals)
	STD_PHP_INI_ENTRY("iocron.mysql_port", "3306", PHP_INI_ALL, OnUpdateString, mysql_port, zend_iocron_globals, iocron_globals)
	STD_PHP_INI_ENTRY("iocron.mysql_host_name", "", PHP_INI_ALL, OnUpdateString, mysql_host_name, zend_iocron_globals, iocron_globals)
PHP_INI_END()
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_iocron_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_iocron_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "iocron", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/

/* {{{ IOCron::__construct(string jobId)
   Creates a IOCron object */
ZEND_BEGIN_ARG_INFO_EX(IOCron___construct_arg_info, 0, 0, 1)
  ZEND_ARG_INFO(0, jobId)
ZEND_END_ARG_INFO()
static PHP_METHOD(IOCron, __construct) {

	iocron_objects *objval = IOCRON_FETCH_OBJECTS(getThis());
	char *jobIdValue;
	size_t jobIdValueLen;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &jobIdValue, &jobIdValueLen) == FAILURE) {
	    return;
	}

	if(iocron_connectDatabase(objval->dbConnection TSRMLS_CC, IOCRON_G(mysql_host_name),
			IOCRON_G(mysql_port), IOCRON_G(mysql_database_name), IOCRON_G(mysql_user_name),
			IOCRON_G(mysql_password)) == IOCRON_PDO_CONNECTION_FAILURE) {

		if(objval->dbConnection->errorMessage.len > 0) {

			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", objval->dbConnection->errorMessage.message);

		}else{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured.");
		}

		return;
	}

	size_t i = 0;
	size_t trimmedStringLen = 0;

	size_t jobIdStrLen = strlen(jobIdValue);
	char *trimmedJobId = emalloc(jobIdStrLen + 2);

	for(i = 0; i < jobIdStrLen; i++) {

		int currentChar = jobIdValue[i];

		if(currentChar >= 48 && currentChar <= 122) {

			memset(&trimmedJobId[trimmedStringLen++], currentChar, sizeof(char));
		}
	}

	memset(&trimmedJobId[trimmedStringLen++], '\0', sizeof(char));
	memset(&trimmedJobId[trimmedStringLen], '\0', sizeof(char));

	if(trimmedStringLen < 3) {

		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Job id %s is too short.", trimmedJobId);
		efree(trimmedJobId);
		return;
	}

	if(trimmedStringLen > 60) {

		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Job id %s is too long. Maximum 60 characters.", trimmedJobId);
		efree(trimmedJobId);
		return;
	}

	objval->objectId = estrndup(trimmedJobId, trimmedStringLen);
	objval->objectIdLen = trimmedStringLen;
	efree(trimmedJobId);

	char *documentRootDir;
	size_t documentRootDirLen = -1;

	if (PG(http_globals)[TRACK_VARS_SERVER] || zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC)) {
		zval **ppzval;

		IOCRON_DEBUG("$_SERVER exists!");

		if(zend_hash_find(Z_ARRVAL_P(PG(http_globals)[TRACK_VARS_SERVER]), "DOCUMENT_ROOT",
				sizeof("DOCUMENT_ROOT"), (void **)&ppzval) == SUCCESS) {

			IOCRON_DEBUG("$_SERVER[DOCUMENT_ROOT] hash finded !");
			documentRootDir = estrndup(Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval));
			documentRootDirLen = Z_STRLEN_PP(ppzval);
		}else{
			IOCRON_DEBUG("$_SERVER[DOCUMENT_ROOT] is not defined!");
		}
	}else{
		IOCRON_DEBUG("$_SERVER not exists!");
	}

	if(documentRootDirLen > 0) {

		IOCRON_DEBUG(documentRootDir);

		size_t memSize = 0;
		char *getJob_queryString = "SELECT id, group_id FROM jobs WHERE jobId = '%s';";
		memSize = snprintf(NULL, 0, getJob_queryString, "");
		char *getJob_queryStringBuffer = emalloc(memSize + 1 + objval->objectIdLen);
		sprintf(getJob_queryStringBuffer, getJob_queryString, objval->objectId);
		zval *jobQuery = iocron_executePDOQuery(objval->dbConnection, (char **)&getJob_queryStringBuffer TSRMLS_CC);
		zval *jobData = NULL;
		jobData = iocron_fetchPDOResult(jobQuery TSRMLS_CC);
		zval **jobDataContent;

		if(zend_hash_index_find(Z_ARRVAL_P(jobData), 0, (void **)&jobDataContent) == SUCCESS) {

			zval **jobId;
			zval **jobGroupId;

			if(zend_hash_index_find(Z_ARRVAL_PP(jobDataContent), 0, (void **)&jobId) == SUCCESS) {

				convert_to_long(*jobId);
				objval->cron_job_id = Z_LVAL_PP(jobId);
			}

			if(zend_hash_index_find(Z_ARRVAL_PP(jobDataContent), 1, (void **)&jobGroupId) == SUCCESS) {

				convert_to_long(*jobGroupId);
				objval->cron_group_id = Z_LVAL_PP(jobGroupId);
			}
		}
		zval_ptr_dtor(&jobData);

		if(objval->cron_job_id == 0) {

			memSize = 0;
			char *getJobGroup_queryString = "SELECT id FROM job_groups WHERE document_root = '%s';";
			memSize = snprintf(NULL, 0, getJobGroup_queryString, "");
			char  *getJobGroup_queryStringBuffer = emalloc(memSize + 1 + documentRootDirLen);
			sprintf(getJobGroup_queryStringBuffer, getJobGroup_queryString, documentRootDir);
			zval *jobGroupQuery = iocron_executePDOQuery(objval->dbConnection, (char **)&getJobGroup_queryStringBuffer TSRMLS_CC);
			zval *groupData = NULL;
			groupData = iocron_fetchPDOResult(jobGroupQuery TSRMLS_CC);
			zval **groupDataContent;

			if(zend_hash_index_find(Z_ARRVAL_P(groupData), 0, (void **)&groupDataContent) == SUCCESS) {

				zval **groupId;

				if(zend_hash_index_find(Z_ARRVAL_PP(groupDataContent), 0, (void **)&groupId) == SUCCESS) {

					convert_to_long(*groupId);
					objval->cron_group_id = Z_LVAL_PP(groupId);
				}
			}

			zval_ptr_dtor(&groupData);

			if(objval->cron_group_id == 0) {

				memSize = 0;
				char *insertJobGroup_queryString = "INSERT INTO job_groups (`document_root`) VALUES ('%s');";
				memSize = snprintf(NULL, 0, insertJobGroup_queryString, "");
				char  *insertJobGroup_queryStringBuffer = emalloc(memSize + 1 + documentRootDirLen);
				sprintf(insertJobGroup_queryStringBuffer, insertJobGroup_queryString, documentRootDir);
				zval *insertJobGroupQuery = iocron_executePDOQuery(objval->dbConnection, (char **)&insertJobGroup_queryStringBuffer TSRMLS_CC);
				iocron_closePDOCursor(insertJobGroupQuery TSRMLS_CC);
				zval *jobGroupId = iocron_getPDOLastInsertId(objval->dbConnection TSRMLS_CC);
				convert_to_long(jobGroupId);
				objval->cron_group_id = Z_LVAL_P(jobGroupId);
				zval_ptr_dtor(&jobGroupId);
			}
		}

		efree(documentRootDir);
	}

	RETURN_TRUE
}
/* }}} */

/* {{{ IOCron::jobExists()
 */
static PHP_METHOD(IOCron, jobExists) {

	iocron_objects *objval = IOCRON_FETCH_OBJECTS(getThis());

	if(objval->cron_job_id == 0) {
		RETURN_BOOL(0);
	}else{
		RETURN_BOOL(1);
	}
}

/* {{{ IOCron::createJob(string $jobFile, string $jobArguments, int $startTime [, bool $isOneTime, string $period ])
   Create a new cron job*/
ZEND_BEGIN_ARG_INFO_EX(IOCron_createJob_arg_info, 0, 0, 3)
	ZEND_ARG_INFO(0, jobFile)
	ZEND_ARG_INFO(0, jobArguments)
	ZEND_ARG_INFO(0, startTime)
	ZEND_ARG_INFO(0, isOneTime)
	ZEND_ARG_INFO(0, period)
ZEND_END_ARG_INFO()
static PHP_METHOD(IOCron, createJob) {

	iocron_objects *objval = IOCRON_FETCH_OBJECTS(getThis());

	char *jobFile;
	size_t jobFileLen;
	char *jobFileArguments;
	size_t jobFileArgumentsLen;
	long startTime;
	zend_bool isOneTime = 0;
	char *period = NULL;
	size_t periodLen = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl|bs", &jobFile, &jobFileLen, &jobFileArguments, &jobFileArgumentsLen,
			&startTime, &isOneTime, &period, &periodLen) == FAILURE) {
	    return;
	}

	if(objval->cron_job_id != 0) {

		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Job exists and running.");
		return;
	}

	char *periodString;
	if(period != NULL && periodLen > 0) {

		size_t periodStringLen = strlen(period);
		periodString = estrndup(period, periodStringLen + 1);
	}else{
		periodString = estrndup("", 1);
	}

	size_t memSize = 0;
	char *insertJob_queryString = "INSERT INTO jobs (`jobId`, `group_id`, `job_file`, `job_args`, \
										`first_start_time`, `next_start_time`, `is_executed`, `one_time_process`, `execute_period`) \
										 VALUES ('%s', %li, '%s', '%s', %li, %li, 0, %i, '%s');";
	memSize = snprintf(NULL, 0, insertJob_queryString, "", objval->cron_group_id, jobFile, jobFileArguments, startTime, startTime,
			isOneTime, periodString);
	char  *insertJob_queryStringBuffer = emalloc(memSize + 1 + objval->objectIdLen);
	sprintf(insertJob_queryStringBuffer, insertJob_queryString, objval->objectId, objval->cron_group_id, jobFile, jobFileArguments,
			startTime, startTime, isOneTime, periodString);

	IOCRON_DEBUG(insertJob_queryStringBuffer);

	zval *insertJobQuery = iocron_executePDOQuery(objval->dbConnection, (char **)&insertJob_queryStringBuffer TSRMLS_CC);
	efree(periodString);

	if (insertJobQuery->type != IS_OBJECT) {
		zval_ptr_dtor(&insertJobQuery);
		RETURN_BOOL(0);
	}

	iocron_closePDOCursor(insertJobQuery TSRMLS_CC);
	zval *jobId = iocron_getPDOLastInsertId(objval->dbConnection TSRMLS_CC);
	convert_to_long(jobId);
	objval->cron_job_id = Z_LVAL_P(jobId);
	zval_ptr_dtor(&jobId);

	RETURN_BOOL(1);
}
/* }}} */

/* {{{ IOCron::updateJob(string $jobFile, string $jobArguments, int $startTime [, bool $isOneTime, string $period ])
   Update a cron job*/
static PHP_METHOD(IOCron, updateJob) {

	iocron_objects *objval = IOCRON_FETCH_OBJECTS(getThis());

	char *jobFile;
	size_t jobFileLen;
	char *jobFileArguments;
	size_t jobFileArgumentsLen;
	long startTime;
	zend_bool isOneTime = 0;
	char *period = NULL;
	size_t periodLen = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl|bs", &jobFile, &jobFileLen, &jobFileArguments, &jobFileArgumentsLen,
				&startTime, &isOneTime, &period, &periodLen) == FAILURE) {
	    return;
	}

	if(objval->cron_job_id == 0) {

		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Job does not exists.");
		return;
	}

	char *periodString;
	if(period != NULL && periodLen > 0) {

		size_t periodStringLen = strlen(period);
		periodString = estrndup(period, periodStringLen + 1);
	}else{
		periodString = estrndup("", 1);
	}

	size_t memSize = 0;
	char *updateJob_queryString = "UPDATE jobs set `job_file` = '%s', `job_args` = '%s', \
										`first_start_time` = %li, `next_start_time` = %li, \
										`is_executed` = 0, `one_time_process` = %i, `execute_period` = '%s' \
										 WHERE id = %li;";
	memSize = snprintf(NULL, 0, updateJob_queryString, jobFile, jobFileArguments, startTime, startTime,
			isOneTime, periodString, objval->cron_job_id);
	char  *updateJob_queryStringBuffer = emalloc(memSize + 1);
	sprintf(updateJob_queryStringBuffer, updateJob_queryString, jobFile, jobFileArguments, startTime, startTime,
			isOneTime, periodString, objval->cron_job_id);

	IOCRON_DEBUG(updateJob_queryStringBuffer);

	zval *updateJobQuery = iocron_executePDOQuery(objval->dbConnection, (char **)&updateJob_queryStringBuffer TSRMLS_CC);
	efree(periodString);

	if (updateJobQuery->type != IS_OBJECT) {
		zval_ptr_dtor(&updateJobQuery);
		RETURN_BOOL(0);
	}

	iocron_closePDOCursor(updateJobQuery TSRMLS_CC);
	RETURN_BOOL(1);
}
/* }}} */

/* {{{ IOCron::deleteJob()
   Delete a cron job*/
static PHP_METHOD(IOCron, deleteJob) {

	iocron_objects *objval = IOCRON_FETCH_OBJECTS(getThis());

	if(objval->cron_job_id == 0) {

		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Job does not exists.");
		return;
	}

	size_t memSize = 0;
	char *deleteJob_queryString = "DELETE FROM jobs WHERE id = %li;";
	memSize = snprintf(NULL, 0, deleteJob_queryString, objval->cron_job_id);
	char  *deleteJob_queryStringBuffer = emalloc(memSize + 1);
	sprintf(deleteJob_queryStringBuffer, deleteJob_queryString, objval->cron_job_id);
	zval *deleteJobQuery = iocron_executePDOQuery(objval->dbConnection, (char **)&deleteJob_queryStringBuffer TSRMLS_CC);

	if (deleteJobQuery->type != IS_OBJECT) {
		zval_ptr_dtor(&deleteJobQuery);
		RETURN_BOOL(0);
	}

	iocron_closePDOCursor(deleteJobQuery TSRMLS_CC);
	RETURN_BOOL(1);
}
/* }}} */

/* {{{ IOCron::getJobLogs([int $limit = 50])
   Get cron job logs */
ZEND_BEGIN_ARG_INFO_EX(IOCron_getJobLogs_arg_info, 0, 0, 1)
	ZEND_ARG_INFO(0, limitValue)
ZEND_END_ARG_INFO()
static PHP_METHOD(IOCron, getJobLogs) {

	iocron_objects *objval = IOCRON_FETCH_OBJECTS(getThis());

	long limitValue = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &limitValue) == FAILURE) {
	    return;
	}

	if(limitValue == 0) {

		limitValue = 50;
	}

	size_t memSize = 0;
	char *listJobLogs_queryString = "SELECT j.jobId, jl.time, jl.execute_status, \
									j.first_start_time, j.next_start_time, j.is_executed, \
									j.one_time_process, j.execute_period \
									FROM job_execute_logs jl \
									LEFT JOIN jobs j \
									ON jl.job_id = j.id \
									WHERE j.group_id = %li \
									ORDER BY jl.time DESC \
									LIMIT 0, %li;";
	memSize = snprintf(NULL, 0, listJobLogs_queryString, objval->cron_group_id, limitValue);
	char  *listJobLogs_queryStringBuffer = emalloc(memSize + 1);
	sprintf(listJobLogs_queryStringBuffer, listJobLogs_queryString, objval->cron_group_id, limitValue);

	IOCRON_DEBUG(listJobLogs_queryStringBuffer);

	zval *listJobLogs = iocron_executePDOQuery(objval->dbConnection, (char **)&listJobLogs_queryStringBuffer TSRMLS_CC);


	if (listJobLogs->type != IS_OBJECT) {
		zval_ptr_dtor(&listJobLogs);
		RETURN_BOOL(0);
	}

	zval *jobLogData = NULL;
	jobLogData = iocron_fetchPDOResult(listJobLogs TSRMLS_CC);
	RETVAL_ZVAL(jobLogData, 1, 1);

	if(jobLogData) {

		zval_ptr_dtor(&jobLogData);
	}
}
/* }}} */

/* {{{ IOCron::listJobs()
   Get cron jobs */
static PHP_METHOD(IOCron, listJobs) {

	iocron_objects *objval = IOCRON_FETCH_OBJECTS(getThis());

	size_t memSize = 0;
	char *listJobs_queryString = "SELECT jobId, job_file, job_args, \
									first_start_time, next_start_time, is_executed, \
									one_time_process, execute_period \
									FROM jobs \
									WHERE group_id = %li \
									ORDER BY id ASC;";
	memSize = snprintf(NULL, 0, listJobs_queryString, objval->cron_group_id);
	char  *listJobs_queryStringBuffer = emalloc(memSize + 1);
	sprintf(listJobs_queryStringBuffer, listJobs_queryString, objval->cron_group_id);
	zval *listJobs = iocron_executePDOQuery(objval->dbConnection, (char **)&listJobs_queryStringBuffer TSRMLS_CC);

	if (listJobs->type != IS_OBJECT) {
		zval_ptr_dtor(&listJobs);
		RETURN_BOOL(0);
	}

	zval *jobsData = NULL;
	jobsData = iocron_fetchPDOResult(listJobs TSRMLS_CC);
	RETVAL_ZVAL(jobsData, 1, 1);


	if(jobsData) {

		zval_ptr_dtor(&jobsData);
	}
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
PHP_MINFO_FUNCTION(iocron) {
	php_info_print_table_start();
	php_info_print_table_header(2, "iocron support", "enabled");
	php_info_print_table_end();

	Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
} */
/* }}} */

/* {{{ iocron_functions[]
 *
 * Every user visible function must have an entry in iocron_functions[].
 */
const zend_function_entry iocron_functions[] = {
	PHP_ME(IOCron, __construct, IOCron___construct_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(IOCron, jobExists, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(IOCron, createJob, IOCron_createJob_arg_info, ZEND_ACC_PUBLIC)
	PHP_ME(IOCron, updateJob, IOCron_createJob_arg_info, ZEND_ACC_PUBLIC)
	PHP_ME(IOCron, deleteJob, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(IOCron, getJobLogs, IOCron_getJobLogs_arg_info, ZEND_ACC_PUBLIC)
	PHP_ME(IOCron, listJobs, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END	/* Must be the last line in serverapi_functions[] */
};
/* }}} */

#if ZEND_MODULE_API_NO >= 20140815

zend_object *iocron_ctor(zend_class_entry *ce) {

	iocron_objects *objval = ecalloc(1, sizeof(iocron_objects));
	zend_object_std_init(&objval->obj, ce);
	objval->obj.handlers = &iocron_handlers;
	objval->dbConnection = ecalloc(1, sizeof(iocron_pdo_connection));
	objval->dbConnection->errorMessage.len = 0;
	objval->cron_group_id = 0;
	objval->cron_job_id = 0;

	return &objval->obj;
}

static zend_object *iocron_clone(zval *zobject) {

	iocron_objects *stmt;
	iocron_objects *old_stmt;

	stmt = ecalloc(1, sizeof(iocron_objects) + zend_object_properties_size(Z_OBJCE_P(zobject)));
	zend_object_std_init(&stmt->obj, Z_OBJCE_P(zobject));
	object_properties_init(&stmt->obj, Z_OBJCE_P(zobject));

	old_stmt = IOCRON_FETCH_OBJECTS(zobject);

	zend_objects_clone_members(&stmt->obj, &old_stmt->obj);

	return &stmt->obj;
}

static void iocron_objects_dtor(zend_object *zobject) /* {{{ */{

	iocron_objects *objval = php_iocron_object(zobject);

	if (objval->objectId) {
		efree(objval->objectId);
	}

	if (objval->dbConnection) {
		iocron_closeConnectionIfExists(objval->dbConnection TSRMLS_CC);
		efree(objval->dbConnection);
	}

	zend_object_std_dtor(&(objval->obj));
	efree(objval);

	//return retval;
}
/* }}} */

#else

static void iocron_objects_dtor(iocron_objects *objval TSRMLS_DC) /* {{{ */{

	if (objval->objectId) {
		efree(objval->objectId);
	}

	if (objval->dbConnection) {
		iocron_closeConnectionIfExists(objval->dbConnection TSRMLS_CC);
		efree(objval->dbConnection);
	}

	zend_object_std_dtor(&(objval->obj) TSRMLS_CC);
	efree(objval);
}
/* }}} */

static zend_object_value iocron_ctor_ex(iocron_objects **pobjval, zend_class_entry *ce TSRMLS_DC) /* {{{ */ {

	iocron_objects *objval = ecalloc(1, sizeof(iocron_objects));
	objval->dbConnection = ecalloc(1, sizeof(iocron_pdo_connection));
	objval->dbConnection->errorMessage.len = 0;
	objval->cron_group_id = 0;
	objval->cron_job_id = 0;
	zend_object_value retval;

	zend_object_std_init(&(objval->obj), ce TSRMLS_CC);
	retval.handle = zend_objects_store_put(objval, NULL, (zend_objects_free_object_storage_t)iocron_objects_dtor, NULL TSRMLS_CC);
	retval.handlers = &iocron_handlers;

	if (pobjval) {
		*pobjval = objval;
	}

	return retval;
}
/* }}} */

static zend_object_value iocron_ctor(zend_class_entry *ce TSRMLS_DC) /* {{{ */ {

	return iocron_ctor_ex(NULL, ce TSRMLS_CC);
}
/* }}} */

static zend_object_value iocron_clone(zval *zobject TSRMLS_DC) /* {{{ */ {

	iocron_objects *old_object = IOCRON_FETCH_OBJECTS(zobject);
	iocron_objects *new_object;
	zend_object_value retval = iocron_ctor_ex(&new_object, old_object->obj.ce TSRMLS_CC);

	zend_objects_clone_members(&(new_object->obj), retval, &(old_object->obj), Z_OBJ_HANDLE_P(zobject) TSRMLS_CC);

	if (old_object->objectId) {
		new_object->objectId = estrndup(old_object->objectId, old_object->objectIdLen);
		new_object->objectIdLen = old_object->objectIdLen;
	}

	new_object->dbConnection = ecalloc(1, sizeof(iocron_pdo_connection));
	new_object->dbConnection->errorMessage.len = 0;
	new_object->cron_group_id = old_object->cron_group_id;
	new_object->cron_job_id = old_object->cron_job_id;

	return retval;
}
/* }}} */
#endif

/* {{{ PHP_GINIT_FUNCTION
 */
PHP_GINIT_FUNCTION(iocron) {
	iocron_globals->mysql_host_name = "";
	iocron_globals->mysql_port = "3306";
	iocron_globals->mysql_user_name = "";
	iocron_globals->mysql_password = "";
	iocron_globals->mysql_database_name = "";
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(iocron)
{
	/* If you have INI entries, uncomment these lines */
	REGISTER_INI_ENTRIES();

	zend_class_entry zce;

	INIT_CLASS_ENTRY(zce, "IOCron", iocron_functions)
	iocron_ce = zend_register_internal_class(&zce TSRMLS_CC);
	iocron_ce->create_object = iocron_ctor;
	memcpy(&iocron_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

#if ZEND_MODULE_API_NO >= 20140815
	iocron_handlers.offset = XtOffsetOf(iocron_objects, obj);
	iocron_handlers.dtor_obj = iocron_objects_dtor;
#else
	iocron_handlers.read_property   = NULL;
	iocron_handlers.has_property    = NULL;
	iocron_handlers.get_property_ptr_ptr = NULL;
#endif

	iocron_handlers.write_property  = NULL;
	iocron_handlers.unset_property  = NULL;
	iocron_handlers.clone_obj = iocron_clone;

	zend_declare_class_constant_string(iocron_ce, "VERSION", sizeof("VERSION") - 1, PHP_IOCRON_VERSION TSRMLS_CC);
	zend_declare_class_constant_string(iocron_ce, "DATABASE_NAME", sizeof("DATABASE_NAME") - 1, IOCRON_G(mysql_database_name) TSRMLS_CC);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(iocron)
{
	/* uncomment this line if you have INI entries
	*/
	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
PHP_RSHUTDOWN_FUNCTION(iocron) {

	return SUCCESS;

}*/
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
PHP_RSHUTDOWN_FUNCTION(serverapi) {
	SERVERAPI_DEBUG("Serverapi Request end !");

	return SUCCESS;
}
*/
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(iocron)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "IOCron Support", "enabled");
	php_info_print_table_row(2, "IOCron Version", PHP_IOCRON_VERSION);
	php_info_print_table_row(2, "IOCron Database Name", IOCRON_G(mysql_database_name));
	php_info_print_table_row(2, "IOCron Database Host Name", IOCRON_G(mysql_host_name));
	php_info_print_table_end();

	php_info_print_box_start(0);
	PUTS("This estension is experimental and just tested on PHP 5.6.x.");
	PUTS("\n<br />\n");
	PUTS("Die Erweiterung ist süß");
	PUTS("\n<br />\n");
	PUTS("Copyright (c) 2015 Ilker Özcan");
	php_info_print_box_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
	//DISPLAY_INI_ENTRIES();
}
/* }}} */

/** {{{ module depends
 */
zend_module_dep iocron_deps[] = {
	ZEND_MOD_CONFLICTS("xdebug")
	ZEND_MOD_REQUIRED("pdo")
	ZEND_MOD_REQUIRED("pdo_mysql")
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ iocron_module_entry
 */
zend_module_entry iocron_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	iocron_deps,
	"iocron",
	NULL,
	PHP_MINIT(iocron),
	PHP_MSHUTDOWN(iocron),
	/* PHP_RINIT(iocron),		 Replace with NULL if there's nothing to do at request start */
	NULL,
	/* PHP_RSHUTDOWN(iocron),	 Replace with NULL if there's nothing to do at request end */
	NULL,
	PHP_MINFO(iocron),
	PHP_IOCRON_VERSION,
	PHP_MODULE_GLOBALS(iocron),
	PHP_GINIT(iocron), /* GINIT */
	NULL,    /* GSHUTDOWN */
	NULL,    /* RPOSTSHUTDOWN */
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_IOCRON
ZEND_GET_MODULE(iocron)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
