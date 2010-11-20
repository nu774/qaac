#ifndef _STDINT_H
#define _STDINT_H

#include <intsafe.h>
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
typedef signed __int64 int64_t;
typedef signed __int32 int32_t;
typedef signed __int16 int16_t;
typedef signed __int8  int8_t;

typedef unsigned __int64 uint_least64_t;
typedef unsigned __int32 uint_least32_t;
typedef unsigned __int16 uint_least16_t;
typedef unsigned __int8 uint_least8_t;
typedef signed __int64 int_least64_t;
typedef signed __int32 int_least32_t;
typedef signed __int16 int_least16_t;
typedef signed __int8  int_least8_t;

typedef unsigned __int64 uint_fast64_t;
typedef unsigned __int32 uint_fast32_t;
typedef unsigned __int16 uint_fast16_t;
typedef unsigned __int8 uint_fast8_t;
typedef signed __int64 int_fast64_t;
typedef signed __int32 int_fast32_t;
typedef signed __int16 int_fast16_t;
typedef signed __int8  int_fast8_t;

/*
typedef LONG_PTR intptr_t;
typedef DWORD_PTR uintptr_t;
*/

typedef __int64  intmax_t;
typedef unsigned __int64   uintmax_t;

#if !defined ( __cplusplus) || defined (__STDC_LIMIT_MACROS)

#define INT_LEAST8_MIN INT8_MIN
#define INT_LEAST16_MIN INT16_MIN
#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST64_MIN INT64_MIN

#define INT_LEAST8_MAX INT8_MAX
#define INT_LEAST16_MAX INT16_MAX
#define INT_LEAST32_MAX INT32_MAX
#define INT_LEAST64_MAX INT64_MAX

#define UINT_LEAST8_MAX UINT8_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#define UINT_LEAST64_MAX UINT64_MAX

#define INT_FAST8_MIN INT8_MIN
#define INT_FAST16_MIN INT16_MIN
#define INT_FAST32_MIN INT32_MIN
#define INT_FAST64_MIN INT64_MIN

#define INT_FAST8_MAX INT8_MAX
#define INT_FAST16_MAX INT16_MAX
#define INT_FAST32_MAX INT32_MAX
#define INT_FAST64_MAX INT64_MAX

#define UINT_FAST8_MAX UINT8_MAX
#define UINT_FAST16_MAX UINT16_MAX
#define UINT_FAST32_MAX UINT32_MAX
#define UINT_FAST64_MAX UINT64_MAX

#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX
#define UINTMAX_MAX UINT64_MAX

#define PTRDIFF_MIN INT32_MIN
#define PTRDIFF_MAX INT32_MAX

#define SIG_ATOMIC_MIN INT32_MIN
#define SIG_ATOMIC_MAX INT32_MAX

/* #define SIZE_MAX UINT32_MAX */

#ifndef WCHAR_MIN  /* also in wchar.h */ 
#define WCHAR_MIN 0
#define WCHAR_MAX ((wchar_t)-1) /* UINT16_MAX */
#endif

/*
 * wint_t is unsigned int in __MINGW32__,
 * but unsigned short in MS runtime
 */
#define WINT_MIN 0
#define WINT_MAX UINT32_MAX

#endif /* !defined ( __cplusplus) || defined __STDC_LIMIT_MACROS */


/* 7.18.4  Macros for integer constants */
#if !defined ( __cplusplus) || defined (__STDC_CONSTANT_MACROS)

/* 7.18.4.1  Macros for minimum-width integer constants

    Accoding to Douglas Gwyn <gwyn@arl.mil>:
	"This spec was changed in ISO/IEC 9899:1999 TC1; in ISO/IEC
	9899:1999 as initially published, the expansion was required
	to be an integer constant of precisely matching type, which
	is impossible to accomplish for the shorter types on most
	platforms, because C99 provides no standard way to designate
	an integer constant with width less than that of type int.
	TC1 changed this to require just an integer constant
	*expression* with *promoted* type."

	The trick used here is from Clive D W Feather.
*/

#define INT8_C(val) (INT_LEAST8_MAX-INT_LEAST8_MAX+(val))
#define INT16_C(val) (INT_LEAST16_MAX-INT_LEAST16_MAX+(val))
#define INT32_C(val) (INT_LEAST32_MAX-INT_LEAST32_MAX+(val))
#define INT64_C(val) (INT_LEAST64_MAX-INT_LEAST64_MAX+(val))

#define UINT8_C(val) (UINT_LEAST8_MAX-UINT_LEAST8_MAX+(val))
#define UINT16_C(val) (UINT_LEAST16_MAX-UINT_LEAST16_MAX+(val))
#define UINT32_C(val) (UINT_LEAST32_MAX-UINT_LEAST32_MAX+(val))
#define UINT64_C(val) (UINT_LEAST64_MAX-UINT_LEAST64_MAX+(val))

/* 7.18.4.2  Macros for greatest-width integer constants */
#define INTMAX_C(val) (INTMAX_MAX-INTMAX_MAX+(val))
#define UINTMAX_C(val) (UINTMAX_MAX-UINTMAX_MAX+(val))

#endif  /* !defined ( __cplusplus) || defined __STDC_CONSTANT_MACROS */

#endif
