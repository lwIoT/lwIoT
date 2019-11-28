/*
 * E/STACK error codes
 *
 * Author: Michel Megens
 * Date: 04/12/2017
 * Email: dev@bietje.net
 */

#ifndef __ESTACK_ERROR_H__
#define __ESTACK_ERROR_H__

/**
 * @brief lwIoT error codes.
 * @ingroup util
 */
typedef enum error_code {
	EOK = 0,
	EDRPPPED,
	EARRIVED,
	EINVALID,
	ENOMEMORY,
	EINUSE,
	ENOTSUPPORTED,
	ENOSOCK,
	ETMO,
	ETRYAGAIN,
	EISCONNECTED,
	ENOTFOUND
} lwiot_error_t;

#define ETIMEOUT ETMO

#endif // !__ESTACK_ERROR_H__
