/*
 * $Id$
 *
 *   * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 24, 2011, 11:13 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_UTIL_GENERALDEFINES_HH
#define	EXFEL_UTIL_GENERALDEFINES_HH

namespace exfel {
  namespace util {
    
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
    
#if defined(_WIN32) && defined(__DLL__)
#define EXFEL_API __declspec(dllexport)
#define TEMPLATE_DLL
#elif defined(unix)        || defined(__unix)      || defined(__unix__) \
 || defined(linux)       || defined(__linux)     || defined(__linux__) \
 || defined(sun)         || defined(__sun) \
 || defined(BSD)         || defined(__OpenBSD__) || defined(__NetBSD__) \
 || defined(__FreeBSD__) || defined __DragonFly__ \
 || defined(sgi)         || defined(__sgi) \
 || defined(__MACOSX__)  || defined(__APPLE__) \
 || defined(__CYGWIN__)
#define EXFEL_API
#else
#define EXFEL_API __declspec(dllimport)
#define TEMPLATE_DLL extern
#endif

#if defined(_WIN32)
    typedef __unit64 uint64
#else
    typedef unsigned long long uint64;
#endif
    
  }
}



#endif	/* EXFEL_PACKAGENAME_GENERALDEFINES_HH */

