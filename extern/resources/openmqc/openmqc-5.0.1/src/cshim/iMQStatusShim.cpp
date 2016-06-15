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
 * @(#)iMQStatusShim.cpp	1.13 06/26/07
 */ 

#include "mqstatus.h"
#include "../error/ErrorTrace.h"
#include "../util/UtilityMacros.h"
#include "../util/LogUtils.hpp"

const MQStatus MQ_STATUS_SUCCESS = {MQ_SUCCESS};

/*
 *
 */
EXPORTED_SYMBOL MQBool
MQStatusIsError(const MQStatus status)
{
  return status.errorCode != MQ_SUCCESS;
}

/*
 *
 */
EXPORTED_SYMBOL MQError
MQGetStatusCode(const MQStatus status)
{
  return status.errorCode;
}

/*
 *
 */
EXPORTED_SYMBOL MQString
MQGetStatusString(const MQStatus status)
{
  const ConstMQString errorString = errorStr(status.errorCode);
  MQString returnString = new MQChar[STRLEN(errorString)+1];
  if (returnString == NULL) {
    return NULL;
  }
  STRCPY(returnString, errorString);
      
  return returnString;
}


EXPORTED_SYMBOL void 
MQFreeString(MQString statusString)
{
  if (statusString != NULL) {
    delete[] (MQString)statusString;
  }
}


EXPORTED_SYMBOL MQString 
MQGetErrorTrace()
{
  MQString returnString = NULL;

#ifndef MQ_NO_ERROR_TRACE
  ErrorTrace *trace = NULL;
  size_t size = 0, i = 0;

  if (getErrorTrace(&trace) != PR_SUCCESS || trace == NULL) {
    return NULL; //XXX return some error in string ?
  }

  for (i = 0; i < trace->num_elements; i++) { 
     if (trace->trace[i] ==  NULL) break;
     size += STRLEN(trace->trace[i]);
     size += 2;
  }

  returnString = new MQChar[size+1];
  if (returnString == NULL) {
    return NULL; //XXX
  }
  returnString[0] = '\0';
  for (i = 0; i < trace->num_elements; i++) { 
    if (trace->trace[i] ==  NULL) break;
    STRCAT(returnString, trace->trace[i]);
    STRCAT(returnString, "\n");
  }
#endif
  return returnString; 
}
