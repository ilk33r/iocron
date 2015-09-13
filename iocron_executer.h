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

#ifndef EXT_IOCRON_IOCRON_EXECUTER_H_
#define EXT_IOCRON_IOCRON_EXECUTER_H_

#ifdef ZTS
#include "TSRM.h"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef TRUE
#define TRUE 1
#endif

#include "php_php_iocron_version.h"

#ifndef PHP_IOCRON_VERSION
#	define PHP_IOCRON_VERSION "~ Dev"
#endif

#ifndef PHP_IOCRON_PHP_BINARY_PATH
#	define PHP_IOCRON_PHP_BINARY_PATH "/usr/bin/php"
#endif

#ifndef PHP_IOCRON_PHP_PREFIX
#	define PHP_IOCRON_PHP_PREFIX "/usr/local/php/php5.6.11"
#endif

#endif /* EXT_IOCRON_IOCRON_EXECUTER_H_ */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
