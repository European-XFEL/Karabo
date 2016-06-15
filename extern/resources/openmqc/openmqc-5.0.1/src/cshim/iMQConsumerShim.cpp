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
 * @(#)iMQConsumerShim.cpp	1.14 06/26/07
 */ 

#include "mqconsumer.h"
#include "shimUtils.hpp"
#include "../client/MessageConsumer.hpp"
#include "../client/Session.hpp"
#include "../client/Message.hpp"
#include "../io/PacketType.hpp"


/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQCloseMessageConsumer(MQConsumerHandle consumerHandle)
{
  static const char FUNCNAME[] = "MQCloseMessageConsumer";
  MQError errorCode = MQ_SUCCESS;
  Session * session = NULL;
  MessageConsumer * consumer = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);
  
  // Convert consumerHandle to a MessageConsumer pointer
  consumer = (MessageConsumer*)getHandledObject(consumerHandle.handle, 
                                                MESSAGE_CONSUMER_OBJECT);
  CNDCHK( consumer == NULL, MQ_STATUS_INVALID_HANDLE);

  // Close the consumer via the session
  session = consumer->getSession();
  if (session != NULL) {
    // This won't actually delete the destination because this function
    // still owns a pointer to it.
    ERRCHK( session->closeConsumer(consumer) );
  }

  // Release our pointer to the object, this actually deletes the consumer
  releaseHandledObject(consumer);

  // This only has an effect if consumer->getSession() was NULL, which
  // should never happen.
  // freeHandledObject(consumerHandle.handle, MESSAGE_CONSUMER_OBJECT);

  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(consumer);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQReceiveMessageWait(const MQConsumerHandle consumerHandle, 
                       MQMessageHandle *      messageHandle)
{
  return MQReceiveMessageWithTimeout(consumerHandle, 
                                       PR_INTERVAL_NO_TIMEOUT,
                                       messageHandle);
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQReceiveMessageNoWait(const MQConsumerHandle consumerHandle, 
                         MQMessageHandle *      messageHandle)
{
  static const char FUNCNAME[] = "MQReceiveMessageNoWait";
  MQError errorCode = MQ_SUCCESS;
  MessageConsumer * consumer = NULL;
  Message * message = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);

  CNDCHK( messageHandle == NULL, MQ_NULL_PTR_ARG );
  messageHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Convert consumerHandle to a MessageConsumer pointer
  consumer = (MessageConsumer*)getHandledObject(consumerHandle.handle,
                                                MESSAGE_CONSUMER_OBJECT);
  CNDCHK( consumer == NULL, MQ_STATUS_INVALID_HANDLE);

  CNDCHK( consumer->getReceiveMode() != SESSION_SYNC_RECEIVE, MQ_NOT_SYNC_RECEIVE_MODE );
  ERRCHK( consumer->receive(&message, PR_INTERVAL_NO_WAIT) );

  // Export the message
  message->setIsExported(PR_TRUE);
  messageHandle->handle = message->getHandle();

  releaseHandledObject(consumer);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(consumer);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQReceiveMessageWithTimeout(const MQConsumerHandle consumerHandle, 
                              MQInt32 timeoutMilliSeconds,
                              MQMessageHandle *      messageHandle)
{
  static const char FUNCNAME[] = "MQReceiveMessageWithTimeout";
  MQError errorCode = MQ_SUCCESS;
  MessageConsumer * consumer = NULL;
  Message * message = NULL;
                                                                  
  CLEAR_ERROR_TRACE(PR_FALSE);

  CNDCHK( messageHandle == NULL, MQ_NULL_PTR_ARG );
  messageHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Convert consumerHandle to a MessageConsumer pointer
  consumer = (MessageConsumer*)getHandledObject(consumerHandle.handle, 
                                                MESSAGE_CONSUMER_OBJECT);
  CNDCHK( consumer == NULL, MQ_STATUS_INVALID_HANDLE);

  CNDCHK( consumer->getReceiveMode() != SESSION_SYNC_RECEIVE, MQ_NOT_SYNC_RECEIVE_MODE );
  // Block until an error occurs or the next message is received
  if (timeoutMilliSeconds == 0) {
  ERRCHK( consumer->receive(&message,  PR_INTERVAL_NO_TIMEOUT) );
  } else {
  ERRCHK( consumer->receive(&message, timeoutMilliSeconds) );
  }

  // Export the message 
  message->setIsExported(PR_TRUE);
  messageHandle->handle = message->getHandle();

  releaseHandledObject(consumer);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(consumer);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

