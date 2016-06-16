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
 * @(#)iMQTextMessageShim.cpp	1.9 06/26/07
 */ 

#include "mqproperties.h"
#include "mqtext-message.h"
#include "shimUtils.hpp"
#include "../client/TextMessage.hpp"
#include "../client/MessageConsumer.hpp"

/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQCreateTextMessage(MQMessageHandle * messageHandle)
{
   static const char FUNCNAME[] = "MQCreateTextMessage";
  MQError errorCode = MQ_SUCCESS;
  TextMessage * textMessage = NULL;
  
  CLEAR_ERROR_TRACE(PR_FALSE);

  CNDCHK( messageHandle == NULL, MQ_NULL_PTR_ARG );
  messageHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Create a new TextMessage
  MEMCHK( textMessage = new TextMessage() );
  ERRCHK( textMessage->getInitializationError() );
  
  // Make the TextMessage into a valid handle
  textMessage->setIsExported(PR_TRUE);
  messageHandle->handle = textMessage->getHandle();
  
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  DELETE( textMessage );
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQGetTextMessageText(const MQMessageHandle messageHandle,
                       ConstMQString *       messageText)
{
  static const char FUNCNAME[] = "MQGetTextMessageText";
  MQError errorCode = MQ_SUCCESS;
  TextMessage * textMessage = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);
                                                               
  // Make sure messageText is not NULL and then initialize it
  CNDCHK( messageText == NULL, MQ_NULL_PTR_ARG );
  *messageText = NULL;
  
  // Convert messageHandle to a TextMessage pointer
  textMessage = (TextMessage*)getHandledObject(messageHandle.handle,
                                               TEXT_MESSAGE_OBJECT);
  CNDCHK( textMessage == NULL, MQ_STATUS_INVALID_HANDLE );

  ERRCHK( textMessage->getMessageTextString(messageText) );

  releaseHandledObject(textMessage);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(textMessage);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQSetTextMessageText(const MQMessageHandle messageHandle,
                       ConstMQString         messageText)
{
  static const char FUNCNAME[] = "MQSetTextMessageText";
  MQError errorCode = MQ_SUCCESS;
  TextMessage * textMessage = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);
                                                               
  // Make sure messageText is not NULL and then initialize it
  CNDCHK( messageText == NULL, MQ_NULL_PTR_ARG );
  
  // Convert messageHandle to a TextMessage pointer
  textMessage = (TextMessage*)getHandledObject(messageHandle.handle,
                                               TEXT_MESSAGE_OBJECT);
  CNDCHK( textMessage == NULL, MQ_STATUS_INVALID_HANDLE );
  
  ERRCHK( textMessage->setMessageTextString(messageText) );

  releaseHandledObject(textMessage);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(textMessage);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}
