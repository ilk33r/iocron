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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "php.h"
#include "ext/standard/php_string.h"
#include "iocron_executer.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_libmysql_compat.h"

#ifdef ZTS
	void ***tsrm_ls;
#endif

unsigned long lastExecutionTime = 0;

typedef struct _iocron_executer_parameters {

	char *pidfile;
	size_t pidFileLen;
	char *user;
	size_t userLen;
	char *group;
	size_t groupLen;
	long maxProcessCount;
	char *mysqlHostName;
	size_t mysqlHostNameLen;
	char *mysqlPort;
	size_t mysqlPortLen;
	char *mysqlUserName;
	size_t mysqlUserNameLen;
	char *mysqlPassword;
	size_t mysqlPasswordLen;
	char *mysqlDatabaseName;
	size_t mysqlDatabaseNameLen;
	long logging;

} iocron_executer_parameters;

iocron_executer_parameters *iocronExecuterGlobals;

static void showUsage() /* {{{ */ {
	printf("Usage iocronexecuter [iocron_executer ini path] \n\n");
}
/* }}} */

static unsigned long iocron_getCurrentUnixTime() /* {{{ */ {

	return (unsigned long)time(NULL);
}
/* }}} */

static void iocron_ExecuterGlobalsDtor() /* {{{ */{

	if(iocronExecuterGlobals->pidFileLen) {
		free(iocronExecuterGlobals->pidfile);
	}

	if(iocronExecuterGlobals->userLen) {
		free(iocronExecuterGlobals->user);
	}

	if(iocronExecuterGlobals->groupLen) {
		free(iocronExecuterGlobals->group);
	}

	if(iocronExecuterGlobals->mysqlHostNameLen) {
		free(iocronExecuterGlobals->mysqlHostName);
	}

	if(iocronExecuterGlobals->mysqlPortLen) {
		free(iocronExecuterGlobals->mysqlPort);
	}

	if(iocronExecuterGlobals->mysqlUserNameLen) {
		free(iocronExecuterGlobals->mysqlUserName);
	}

	if(iocronExecuterGlobals->mysqlPasswordLen) {
		free(iocronExecuterGlobals->mysqlPassword);
	}

	if(iocronExecuterGlobals->mysqlDatabaseNameLen) {
		free(iocronExecuterGlobals->mysqlDatabaseName);
	}
	free(iocronExecuterGlobals);
}
/* }}} */

static unsigned short iocron_parseIniFile(const char *iniFilePath) /* {{{ */ {

	if( access( iniFilePath, F_OK ) == -1 ) {

		return 0; //could not open file
	}

	FILE *file = fopen(iniFilePath, "r");
	unsigned short keyFound = 0;
	unsigned short valueFound = 0;
	unsigned short keyMallocked = 0;
	unsigned short valueMallocked = 0;
	unsigned short isEscapedLine = 0;
	unsigned short hasIniError = 0;
	char *key;
	char *value;
	size_t currentMemSize = 64;
	size_t keySize = 0;
	size_t valueSize = 0;
	int c;

	if (file == NULL) {

		return 0; //could not open file
	}

	iocronExecuterGlobals = malloc(sizeof(iocron_executer_parameters));
	iocronExecuterGlobals->pidFileLen = 0;
	iocronExecuterGlobals->userLen = 0;
	iocronExecuterGlobals->groupLen = 0;
	iocronExecuterGlobals->mysqlHostNameLen = 0;
	iocronExecuterGlobals->mysqlPortLen = 0;
	iocronExecuterGlobals->mysqlUserNameLen = 0;
	iocronExecuterGlobals->mysqlPasswordLen = 0;
	iocronExecuterGlobals->mysqlDatabaseNameLen = 0;
	iocronExecuterGlobals->logging = 0;
	iocronExecuterGlobals->maxProcessCount = 3;

	while ((c = fgetc(file)) != EOF) {

		if(c == 59 && keyFound == 0 && valueFound == 0 && isEscapedLine == 0) {

			isEscapedLine = 1;
		}

		if(isEscapedLine == 1) {

			if(c == 10 || c == 13) {

				isEscapedLine = 0;
			}

			continue;
		}

		if(keyFound == 0) {

			if(keyMallocked == 0) {

				keyMallocked = 1;
				currentMemSize = 64;
				key = malloc(currentMemSize);
			}

			if(keySize > (currentMemSize - 8)) {

				currentMemSize += 64;
				key = realloc(key, currentMemSize);
			}

			if(keySize > 0) {

				if(c < 97 || c > 122) {

					keyFound = 1;
					memset(&key[keySize++], '\0', sizeof(char));
					memset(&key[keySize], '\0', sizeof(char));
				}
			}else{

				if(c < 97 || c > 122) {

					continue;
				}
			}

			memset(&key[keySize++], c, sizeof(char));

		}else if(keyFound == 1 && valueFound == 0) {

			if(valueMallocked == 0) {

				valueMallocked = 1;
				currentMemSize = 64;
				value = malloc(currentMemSize);
			}

			if(valueSize > (currentMemSize - 8)) {

				currentMemSize += 64;
				value = realloc(value, currentMemSize);
			}

			//Current character is " value 34

			if(valueSize > 0) {

				if(c == 34 || c == 32 || c == 9 || c == 10 || c == 13) {

					valueFound = 1;
					memset(&value[valueSize++], '\0', sizeof(char));
					memset(&value[valueSize], '\0', sizeof(char));
				}
			}else{

				if(c == 34 || c == 32 || c == 9) {

					continue;
				}
			}

			memset(&value[valueSize++], c, sizeof(char));

		}else if(keyFound == 1 && valueFound == 1) {

			if(strcmp(key, "pidfile") == 0) {

				iocronExecuterGlobals->pidfile = strdup(value);
				iocronExecuterGlobals->pidFileLen = valueSize;
			}else if(strcmp(key, "user") == 0) {

				iocronExecuterGlobals->user = strdup(value);
				iocronExecuterGlobals->userLen = valueSize;
			}else if(strcmp(key, "group") == 0) {

				iocronExecuterGlobals->group = strdup(value);
				iocronExecuterGlobals->groupLen = valueSize;
			}else if(strcmp(key, "max_process_count") == 0) {

				iocronExecuterGlobals->maxProcessCount = strtol(value, NULL, 10);
			}else if(strcmp(key, "mysql_host_name") == 0) {

				iocronExecuterGlobals->mysqlHostName = strdup(value);
				iocronExecuterGlobals->mysqlHostNameLen = valueSize;
			}else if(strcmp(key, "mysql_port") == 0) {

				iocronExecuterGlobals->mysqlPort = strdup(value);
				iocronExecuterGlobals->mysqlPortLen = valueSize;
			}else if(strcmp(key, "mysql_user_name") == 0) {

				iocronExecuterGlobals->mysqlUserName = strdup(value);
				iocronExecuterGlobals->mysqlUserNameLen = valueSize;
			}else if(strcmp(key, "mysql_password") == 0) {

				iocronExecuterGlobals->mysqlPassword = strdup(value);
				iocronExecuterGlobals->mysqlPasswordLen = valueSize;
			}else if(strcmp(key, "mysql_database_name") == 0) {

				iocronExecuterGlobals->mysqlDatabaseName = strdup(value);
				iocronExecuterGlobals->mysqlDatabaseNameLen = valueSize;
			}else if(strcmp(key, "database_logging") == 0) {

				iocronExecuterGlobals->logging = strtol(value, NULL, 10);
			}else{
				printf("Warning! The %s does not recognized in ini file. \n", key);
				hasIniError = 1;
				break;
			}

			free(key);
			free(value);
			currentMemSize = 64;
			keySize = 0;
			valueSize = 0;
			keyFound = 0;
			valueFound = 0;
			keyMallocked = 0;
			valueMallocked = 0;
		}
    }

	fclose(file);
	free(key);
	free(value);

	if(hasIniError == 1) {
		iocron_ExecuterGlobalsDtor();
		return 2;
	}

	if(iocronExecuterGlobals->pidFileLen == 0) {
		iocron_ExecuterGlobalsDtor();
		printf("Warning! Pid file value could not be found \n");
		return 2;
	}

	if(iocronExecuterGlobals->userLen == 0) {

		iocronExecuterGlobals->user = strdup("www-data");
		iocronExecuterGlobals->userLen = 9;
	}

	if(iocronExecuterGlobals->groupLen == 0) {

		iocronExecuterGlobals->group = strdup("www-data");
		iocronExecuterGlobals->groupLen = 9;
	}

	if(iocronExecuterGlobals->mysqlPortLen == 0) {

		iocronExecuterGlobals->mysqlPort = strdup("3306");
		iocronExecuterGlobals->mysqlPortLen = 5;
	}

	return 1;
}
/* }}} */

static unsigned short iocron_checkMysqlConnection(TSRMLS_D) /* {{{ */ {

	MYSQLND *iocronconnection;
	if( !( iocronconnection = mysqlnd_init(MYSQLND_CLIENT_NO_FLAG, TRUE) ) ) {
		return 1;
	}

	unsigned long portNumberLong = strtoul(iocronExecuterGlobals->mysqlPort, NULL, 10);

	if(mysqlnd_connect(iocronconnection, iocronExecuterGlobals->mysqlHostName, iocronExecuterGlobals->mysqlUserName,
			iocronExecuterGlobals->mysqlPassword, strlen(iocronExecuterGlobals->mysqlPassword),
			iocronExecuterGlobals->mysqlDatabaseName, strlen(iocronExecuterGlobals->mysqlDatabaseName),
			portNumberLong, NULL, 0, MYSQLND_CLIENT_KNOWS_RSET_COPY_DATA TSRMLS_CC) == NULL) {

		mysqlnd_close(iocronconnection, MYSQLND_CLOSE_DISCONNECTED);

		return 1;
	}


	return 0;
}
/* }}} */

int main(int argc, const char * argv[]) /* {{{ */ {

	if(argc != 2) {
		showUsage();
		return 0;
	}

	unsigned short iniFileStatus = iocron_parseIniFile(argv[1]);
	if(iniFileStatus == 0) {
		printf("Warning ! \n Executor ini file could not be found. \n");
		return 0;
	}else if(iniFileStatus == 2) {
		printf("Warning ! \n Executor ini file could not be resolved. \n");
		return 0;
	}

#ifdef ZTS
	tsrm_startup(1, 1, 0, NULL);
	tsrm_ls = ts_resource(0);
#endif

	if(iocron_checkMysqlConnection(TSRMLS_C) == 1) {
		printf("Warning ! \n Unable to the connect mysql server. \n");
		iocron_ExecuterGlobalsDtor();
		return 0;
	}

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
