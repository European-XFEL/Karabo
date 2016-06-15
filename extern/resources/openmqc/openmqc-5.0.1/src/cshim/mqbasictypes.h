/*
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS HEADER.
 *
 * Copyright (c) 2000-2010 Oracle and/or its affiliates. All rights reserved.
 *
 * The contents of this file are subject to the terms of either the GNU
 * General Public License Version 2 only ("GPL") or the Common Development
 * and Distribution License("CDDL") (collectively, the "License").  You
 * may not use this file except in compliance with the License.  You can
 * obtain a copy of the License at
 * https://glassfish.dev.java.net/public/CDDL+GPL_1_1.html
 * or packager/legal/LICENSE.txt.  See the License for the specific
 * language governing permissions and limitations under the License.
 *
 * When distributing the software, include this License Header Notice in each
 * file and include the License file at packager/legal/LICENSE.txt.
 *
 * GPL Classpath Exception:
 * Oracle designates this particular file as subject to the "Classpath"
 * exception as provided by Oracle in the GPL Version 2 section of the License
 * file that accompanied this code.
 *
 * Modifications:
 * If applicable, add the following below the License Header, with the fields
 * enclosed by brackets [] replaced by your own identifying information:
 * "Portions Copyright [year] [name of copyright owner]"
 *
 * Contributor(s):
 * If you wish your version of this file to be governed by only the CDDL or
 * only the GPL Version 2, indicate your decision by adding "[Contributor]
 * elects to include this software in this distribution under the [CDDL or GPL
 * Version 2] license."  If you don't indicate a single choice of license, a
 * recipient has the option to distribute your version of this file under
 * either the CDDL, the GPL Version 2 or to extend the choice of license to
 * its licensees as provided above.  However, if you add GPL Version 2 code
 * and therefore, elected the GPL Version 2 license, then the option applies
 * only if the new code is made subject to such option by the copyright
 * holder.
 */

/*
 * @(#)mqbasictypes.h	1.10 06/26/07
 */ 

#ifndef MQ_BASICTYPES_H
#define MQ_BASICTYPES_H

/*
 * defines MQ basic types
 */

#if ((defined(__SUNPRO_CC) && (__SUNPRO_CC_COMPAT == 5)) \
         || defined(__SUNPRO_C)) \
    && defined(__sun) && (defined(__sparc) || (defined(__i386) || (defined(__amd64) || (defined(__x86_64)))))
#ifndef SOLARIS
#define SOLARIS
#endif
#endif

#if (defined(__GNUC__) || defined (__GNUG__)) && defined(__linux__)
#ifndef LINUX
#define LINUX
#endif
#endif

//######hpux-dev######
#if (defined(__hpux))
#ifndef HPUX
#define HPUX
#endif
#endif

#if (defined(__IBMC__) || defined (__IBMCPP__)) && defined(__unix__)
#ifndef AIX 
#define AIX 
#endif
#endif


#if defined(_MSC_VER) && defined(_WIN32)
#ifndef WIN32
#define WIN32
#endif
#endif

#ifdef SOLARIS
#include <inttypes.h>
#endif
#ifdef LINUX
#include <stdint.h>
#endif
//#####hpux-dev#####
#ifdef HPUX
#include <inttypes.h>
#endif

#ifdef AIX 
#include <inttypes.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//#####hpux-dev#####
#if defined(SOLARIS) || defined(LINUX) || defined(HPUX) || defined(AIX)
typedef int32_t   MQBool;
typedef int8_t    MQInt8;
typedef int16_t   MQInt16;
typedef int32_t   MQInt32;
typedef int64_t   MQInt64;
typedef uint32_t  MQUint32;
#elif defined(WIN32)
typedef __int32           MQBool;
typedef __int8            MQInt8;
typedef __int16           MQInt16;
typedef __int32           MQInt32;
typedef __int64           MQInt64;
typedef unsigned __int32  MQUint32;
#else
#error unknown platform
#endif

//#####hpux-dev#####
#if defined(SOLARIS) || defined(LINUX) || defined(WIN32) || defined(HPUX) || defined(AIX)
typedef float   MQFloat32;
typedef double  MQFloat64;
typedef char    MQChar;

#define MQ_TRUE  1
#define MQ_FALSE 0
#else
#error unknown platform
#endif

//#####hpux-dev#####
/** internal use only */ 
#if defined(WIN32)
#if defined(MQ_EXPORT_DLL_SYMBOLS)
#define EXPORTED_SYMBOL __declspec(dllexport)
#else
#define EXPORTED_SYMBOL __declspec(dllimport)
#endif /* defined(MQ_EXPORT_DLL_SYMBOLS) */
#elif defined(SOLARIS) || defined(LINUX) || defined(HPUX) || defined(AIX)
#define EXPORTED_SYMBOL 
#else
#error unknown platform
#endif  /* defined(WIN32) */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MQ_BASICTYPES_H */
