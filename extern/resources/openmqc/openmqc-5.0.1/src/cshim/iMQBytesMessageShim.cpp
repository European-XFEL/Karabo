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
 * @(#)iMQBytesMessageShim.cpp	1.9 06/26/07
 */ 

#include "mqproperties.h"
#include "mqbytes-message.h"
#include "shimUtils.hpp"
#include "../client/BytesMessage.hpp"
#include "../client/MessageConsumer.hpp"

/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQCreateBytesMessage(MQMessageHandle * messageHandle)
{
  static const char FUNCNAME[] = "MQCreateBytesMessage";
  MQError errorCode = MQ_SUCCESS;

  CLEAR_ERROR_TRACE(PR_FALSE);

  BytesMessage * bytesMessage = NULL;
  CNDCHK( messageHandle == NULL, MQ_NULL_PTR_ARG );
  messageHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Create a new BytesMessage
  MEMCHK( bytesMessage = new BytesMessage() );
  ERRCHK( bytesMessage->getInitializationError() );
  
  // Make the BytesMessage into a valid handle
  bytesMessage->setIsExported(PR_TRUE);
  messageHandle->handle = bytesMessage->getHandle();

  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  DELETE( bytesMessage );
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQGetBytesMessageBytes(const MQMessageHandle messageHandle,
                         const MQInt8 ** messageBytes,
                         MQInt32 *       messageBytesSize)
{
  static const char FUNCNAME[] = "MQGetBytesMessageBytes";
  MQError errorCode = MQ_SUCCESS;
  BytesMessage * bytesMessage = NULL;
  
  CLEAR_ERROR_TRACE(PR_FALSE);

  CNDCHK( messageBytes == NULL, MQ_NULL_PTR_ARG );
  CNDCHK( messageBytesSize == NULL, MQ_NULL_PTR_ARG );
  *messageBytes = NULL;
  *messageBytesSize = 0;
  
  // Convert messageHandle to a BytesMessage pointer
  bytesMessage = (BytesMessage*)getHandledObject(messageHandle.handle,
                                                 BYTES_MESSAGE_OBJECT);
  CNDCHK( bytesMessage == NULL, MQ_STATUS_INVALID_HANDLE );

  ERRCHK( bytesMessage->getMessageBytes((const PRUint8**)messageBytes,
                                        (PRInt32 *)messageBytesSize) );
  
  releaseHandledObject(bytesMessage);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(bytesMessage);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQSetBytesMessageBytes(const MQMessageHandle messageHandle,
                         const MQInt8 *        messageBytes,
                         MQInt32               messageBytesSize)
{
  static const char FUNCNAME[] = "MQSetBytesMessageBytes";
  MQError errorCode = MQ_SUCCESS;
  BytesMessage * bytesMessage = NULL;
                                                                  
  CLEAR_ERROR_TRACE(PR_FALSE);

  CNDCHK( messageBytes == NULL, MQ_NULL_PTR_ARG );
  
  // Convert messageHandle to a BytesMessage pointer
   bytesMessage = (BytesMessage*)getHandledObject(messageHandle.handle,
                                                  BYTES_MESSAGE_OBJECT);
  CNDCHK( bytesMessage == NULL, MQ_STATUS_INVALID_HANDLE );

  ERRCHK( bytesMessage->setMessageBytes((const PRUint8*)messageBytes,
                                        messageBytesSize) );

  releaseHandledObject(bytesMessage);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(bytesMessage);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}
