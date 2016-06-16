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
 * @(#)DebugUtils.h	1.9 06/26/07
 */ 

/*
 * This file defines debugging utilities for the iMQ C client.
 */

#ifndef DEBUGUTILS_H
#define DEBUGUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32) 
# pragma warning(disable: 4514) /* warning C4514: 'delete' : unreferenced inline function has been removed */ 
# pragma warning(disable: 4711) /* warning C4711: function 'XXX' selected for automatic inline expansion */ 
#endif 

#ifdef __cplusplus
}
#endif

#if defined(WIN32) 
# include <crtdbg.h>

/* #define _CRTDBG_MAP_ALLOC */
# ifdef _DEBUG
#  define DEBUG_CLIENTBLOCK   new( _CLIENT_BLOCK, __FILE__, __LINE__)
# else
#  define DEBUG_CLIENTBLOCK
# endif /* _DEBUG */
#endif /* defined(WIN32) */


#include "../util/MemAllocTest.h"
//#define TEST_MEMORY_ALLOC_FAILURES
#if defined(WIN32) 
#ifdef _DEBUG 
#  ifdef TEST_MEMORY_ALLOC_FAILURES
/* Log the memory failure for iMQ */
#   ifdef MQ_LIBRARY
#    define new (!mallocSucceeds()) ?  \
       (LOG_FINE_NEW(( CODELOC, MEMORY_LOG_MASK, NULL_CONN_ID, IMQ_OUT_OF_MEMORY, "new failed" )),  \
         NULL) : \
       DEBUG_CLIENTBLOCK
#   else
#    define new (!mallocSucceeds()) ?  NULL: DEBUG_CLIENTBLOCK
#   endif /* IMQ_EXPORT_DLL_SYMBOLS */
#  else
#    define new DEBUG_CLIENTBLOCK
#  endif /* TEST_MEMORY_ALLOC_FAILURES */
#endif /* _DEBUG */
#endif /* defined(WIN32) */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>


#include <assert.h>
#define ASSERT assert


#ifdef __cplusplus
}
#endif

#endif /* DEBUGUTILS_H */


