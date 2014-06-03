/*
 * $Id$
 *
 *   * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 24, 2011, 11:13 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_KARABO_DLL_HH
#define	KARABO_UTIL_KARABO_DLL_HH

/********************************************
 *         Disable Specific Warnings         *
 ********************************************/



/********************************************
 *       Declare a parameter as unused      *
 ********************************************/

#if defined(_WIN32)
#define ALWAYS_UNUSED(x) x;
#else
#define ALWAYS_UNUSED(x)
#endif


/********************************************
 *            Windows DLL stuff             *  
 ********************************************/

#if defined(_WIN32)
  #if defined(__DLL__)
    #define KARABO_DECLSPEC __declspec(dllexport)
    #define KARABO_TEMPLATE_DLL
  #else
    #define KARABO_DECLSPEC __declspec(dllimport)
    #define KARABO_TEMPLATE_DLL extern
  #endif
#elif defined(unix)        || defined(__unix)      || defined(__unix__) \
 || defined(linux)       || defined(__linux)     || defined(__linux__) \
 || defined(sun)         || defined(__sun) \
 || defined(BSD)         || defined(__OpenBSD__) || defined(__NetBSD__) \
 || defined(__FreeBSD__) || defined __DragonFly__ \
 || defined(sgi)         || defined(__sgi) \
 || defined(__MACOSX__)  || defined(__APPLE__) \
 || defined(__CYGWIN__)
    #define KARABO_DECLSPEC
    #define KARABO_TEMPLATE_DLL
#endif

#if defined(_WIN32)
typedef __unit64 uint64
#else
typedef unsigned long long uint64;
#endif

#ifdef __GNUC__
#define KARABO_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define KARABO_DEPRECATED __declspec(deprecated)
#else
#pragma message("WARNING: You need to implement KARABO_DEPRECATED for this compiler")
#define KARABO_DEPRECATED
#endif


#endif

