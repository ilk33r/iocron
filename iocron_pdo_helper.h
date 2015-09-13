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

#ifndef IOCRON_PDO_HELPER
#define IOCRON_PDO_HELPER

#define IOCRON_PDO_CONNECTION_SUCCESS 0
#define IOCRON_PDO_CONNECTION_FAILURE -1

typedef struct _iocron_pdo_connection {
	unsigned short isConnectedDatabase;
	struct {
		char *message;
		int len;
	} errorMessage;

	zval *pdoObject;

} iocron_pdo_connection;

void iocron_closeConnectionIfExists(iocron_pdo_connection *conn TSRMLS_DC);
short iocron_connectDatabase(iocron_pdo_connection *conn TSRMLS_DC, char *mysql_host_name, char *mysql_port, char *mysql_database_name,
		char *mysql_user_name, char *mysql_password);
zval *iocron_executePDOQuery(iocron_pdo_connection *conn, char **query TSRMLS_DC);
zval *iocron_fetchPDOResult(zval *pdoStatement TSRMLS_DC);
void iocron_closePDOCursor(zval *pdoStatement TSRMLS_DC);
zval *iocron_getPDOLastInsertId(iocron_pdo_connection *conn TSRMLS_DC);

#endif	/* IOCRON_PDO_HELPER */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
