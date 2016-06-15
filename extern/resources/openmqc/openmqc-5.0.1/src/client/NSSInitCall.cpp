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
 * %W% %G%
 */ 

#include "../util/LogUtils.hpp"
#include <assert.h>
#include "NSSInitCall.h"
#include "../cshim/mqerrors.h"

#define ASSERT assert

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static PRUintn        _callOnceNSSData_key;

static PRCallOnceType _once_nss_init;
static PRCallOnceType _once_thr_priv;

static SECStatus      _callOnceSECStatus = SECFailure;
static PRErrorCode    _callOncePRError;
static PRErrorCode    _callOncePROSError;

static PRBool         _calledNSS_Init = PR_FALSE;
static PRBool         _calledNSS_NoDB_Init = PR_FALSE;

static void PR_CALLBACK deleteCallOnceNSSData(void * data) 
{
  if (data != NULL) free(data);
}


static PRStatus PR_CALLBACK once_fn_thr_priv(void) 
{

  LOG_INFO(( CODELOC, CONNECTION_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
		    "Preparing for NSS initialization ..." ));

  return PR_NewThreadPrivateIndex(&_callOnceNSSData_key, &deleteCallOnceNSSData);
}


static PRStatus PR_CALLBACK once_fn_nss_init(void) 
{

  LOG_INFO(( CODELOC, CONNECTION_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
		    "Initializing NSS ..." ));

  ASSERT( _calledNSS_Init == PR_FALSE && _calledNSS_NoDB_Init == PR_FALSE );

  CallOnceNSSData * data = (CallOnceNSSData *)PR_GetThreadPrivate(_callOnceNSSData_key);
  if (data == NULL) return PR_FAILURE;

  if (data->noDB == PR_TRUE) {
    _callOnceSECStatus = NSS_NoDB_Init(NULL);
    _calledNSS_NoDB_Init = PR_TRUE;
  } else {
    _callOnceSECStatus = NSS_Init(data->certDir);
    _calledNSS_Init = PR_TRUE;
  }

  _callOncePRError   = PR_GetError();
  _callOncePROSError = PR_GetOSError();

  return PR_SUCCESS;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


SECStatus callOnceNSS_Init(const char * certDir)
{
  if (_once_nss_init.initialized == 0) {

  if (_once_thr_priv.initialized == 0) {
    if(PR_CallOnce(&_once_thr_priv, once_fn_thr_priv) != PR_SUCCESS) return SECFailure;
  }

  if (_once_nss_init.initialized == 0) {

    CallOnceNSSData * data = (CallOnceNSSData *)PR_GetThreadPrivate(_callOnceNSSData_key);
    if (data == NULL) {
      data = (CallOnceNSSData *) malloc(sizeof(CallOnceNSSData));
      if (data == NULL) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        return SECFailure;
      }
      if (PR_SetThreadPrivate(_callOnceNSSData_key, data) != PR_SUCCESS) {
        free(data);
        return SECFailure;
      }
    }

    data->certDir = certDir;
    if (certDir == NULL) {
      data->noDB = PR_TRUE;
    } else {
      data->noDB = PR_FALSE;
    }

    if(PR_CallOnce(&_once_nss_init, once_fn_nss_init) != PR_SUCCESS) return SECFailure;
  }

  }

  PR_SetError(_callOncePRError, _callOncePROSError);
  return _callOnceSECStatus;

}

PRBool calledNSS_Init() {
  ASSERT( (_calledNSS_Init == PR_TRUE && _calledNSS_NoDB_Init == PR_FALSE) || 
          (_calledNSS_Init == PR_FALSE && _calledNSS_NoDB_Init == PR_TRUE) || 
          (_calledNSS_Init == PR_FALSE && _calledNSS_NoDB_Init == PR_FALSE) );

  return _calledNSS_Init;
}
