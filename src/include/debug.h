////////////////////////////////////////////////////////////////////////////////
//
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      debug.h
//
// Description:
//
//      Macros for printing debug messages. Supports 6 debug levels, 5 being the
//      most verbose, 1 being the least verbose. Level 0 is always printed.
//
// Author:
//
//      Mike Kirkland
//
// Created:
//
//      October 2001
//
////////////////////////////////////////////////////////////////////////////////

#ifndef DEBUG_H
#define DEBUG_H

#include <messages.h>
#include <string.h>
#include <stdarg.h>

#define DEBUG_0(x...) itc_printf(x)

#if DEBUG_LEVEL >= 1
#define DEBUG_1(x...) itc_printf(x)
#else
#define DEBUG_1(x...)
#endif

#if DEBUG_LEVEL >= 2
#define DEBUG_2(x...) itc_printf(x)
#else
#define DEBUG_2(x...)
#endif

#if DEBUG_LEVEL >= 3
#define DEBUG_3(x...) itc_printf(x)
#else
#define DEBUG_3(x...)
#endif

#if DEBUG_LEVEL >= 4
#define DEBUG_4(x...) itc_printf(x)
#else
#define DEBUG_4(x...)
#endif

#if DEBUG_LEVEL >= 5
#define DEBUG_5(x...) itc_printf(x)
#else
#define DEBUG_5(x...)
#endif

#ifdef _DEBUG
# define DEBUG(x...) itc_printf(x)

# ifdef _DEBUG_TRACE
#  define DEBUG_TRACE(x...) itc_printf(x)
# else
#  define DEBUG_TRACE(x...)
# endif

# ifdef _DEBUG_LOGIC
#  define DEBUG_LOGIC(x...) itc_printf(x)
# else
#  define DEBUG_LOGIC(x...)
# endif

# ifdef _DEBUG_PARAM
#  define DEBUG_PARAM(x...) itc_printf(x)
# else
#  define DEBUG_PARAM(x...)
# endif

# ifdef _DEBUG_ERROR
#  define DEBUG_ERROR(x...) itc_printf(x)
# else
#  define DEBUG_ERROR(x...)
# endif

# ifdef _DEBUG_FAIL
#  define DEBUG_FAIL(x...) itc_printf(x)
# else
#  define DEBUG_FAIL(x...)
# endif

#define ASSERT(x) if(!(x)){itc_printf("ASSERT failed: (%s).\r\n", #x);}

#else
#define DEBUG(x...) itc_printf(x)
#define DEBUG_TRACE(x...)
#define DEBUG_LOGIC(x...)
#define DEBUG_PARAM(x...)
#define DEBUG_FAIL(x...)
#define DEBUG_ERROR(x...)
#define ASSERT(x)
#endif
#endif //DEBUG_H
