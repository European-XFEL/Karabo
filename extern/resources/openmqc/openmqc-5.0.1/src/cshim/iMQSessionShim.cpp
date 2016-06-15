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
 * @(#)iMQSessionShim.cpp	1.18 06/26/07
 */ 

#include "mqsession.h"
#include "shimUtils.hpp"
#include "../client/Session.hpp"

/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQCloseSession(MQSessionHandle sessionHandle)
{
  static const char FUNCNAME[] = "MQCloseSession";
  MQError errorCode = MQ_SUCCESS;
  Session * session = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);

  // Convert sessionHandle to a Session pointer
  session = (Session*)getHandledObject(sessionHandle.handle, 
                                       SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE);

  // Close the session
  ERRCHK( session->close(PR_TRUE) );

  // Delete the session
  releaseHandledObject(session);
  HANDLED_DELETE( session );
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQCreateDestination(const MQSessionHandle sessionHandle,
                      ConstMQString         destinationName,
                      MQDestinationType     destinationType,
                      MQDestinationHandle * destinationHandle)
{
  static const char FUNCNAME[] = "MQCreateDestination";
  MQError errorCode = MQ_SUCCESS;
  Destination * destination = NULL;
  Session * session = NULL;
  MQBool isQueue; 

  CLEAR_ERROR_TRACE(PR_FALSE);
  
  CNDCHK( destinationName == NULL, MQ_NULL_PTR_ARG );
  CNDCHK( destinationHandle == NULL, MQ_NULL_PTR_ARG );
  destinationHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Convert sessionHandle to a Session pointer
  session = (Session*)getHandledObject(sessionHandle.handle,
                                       SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE ); 
  if (destinationType == MQ_QUEUE_DESTINATION) {
    isQueue = MQ_TRUE;
  } else if (destinationType == MQ_TOPIC_DESTINATION) {
    isQueue = MQ_FALSE;
  } else {
    ERRCHK( MQ_INVALID_DESTINATION_TYPE );
  }
    
  // Create the destination
  {
    UTF8String destinationNameStr(destinationName);
    CNDCHK( STRCMP( destinationNameStr.toString(), destinationName ) != 0, MQ_OUT_OF_MEMORY );
    ERRCHK( session->createDestination(&destinationNameStr, isQueue, &destination) );
  }

  // Make the destination a valid handle
  destination->setIsExported(PR_TRUE);
  destinationHandle->handle = destination->getHandle();

  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  ASSERT( destination == NULL );
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQCreateTemporaryDestination(const MQSessionHandle sessionHandle,
                               MQDestinationType destinationType,
                               MQDestinationHandle * destinationHandle)
{
  static const char FUNCNAME[] = "MQCreateTemporaryDestination";
  MQError errorCode = MQ_SUCCESS;
  Destination * destination = NULL;
  Session * session = NULL;
  MQBool isQueue;
  
  CLEAR_ERROR_TRACE(PR_FALSE);

  CNDCHK( destinationHandle == NULL, MQ_NULL_PTR_ARG );
  destinationHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Convert sessionHandle to a Session pointer
  session = (Session*)getHandledObject(sessionHandle.handle,
                                       SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE ); 
  
  if (destinationType == MQ_QUEUE_DESTINATION) {
    isQueue = MQ_TRUE;
  } else if (destinationType == MQ_TOPIC_DESTINATION) {
    isQueue = MQ_FALSE;
  } else {
    ERRCHK( MQ_INVALID_DESTINATION_TYPE );
  }

   // Create the temporary destination
  ERRCHK( session->createTemporaryDestination(isQueue, &destination) );

  // Make the destination a valid handle
  destination->setIsExported(PR_TRUE);
  destinationHandle->handle = destination->getHandle();

  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  ASSERT( destination == NULL );
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}




/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQCreateMessageProducerForDestination(const MQSessionHandle sessionHandle,
                                      const MQDestinationHandle destinationHandle,
                                      MQProducerHandle *        producerHandle)
{
  static const char FUNCNAME[] = "MQCreateMessageProducerForDestination";
  MQError errorCode = MQ_SUCCESS;
  MessageProducer * producer = NULL;
  Session * session = NULL;
  Destination * destination = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);
    
  CNDCHK( producerHandle == NULL, MQ_NULL_PTR_ARG );
  producerHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Convert sessionHandle to a Session pointer
  session = (Session*)getHandledObject(sessionHandle.handle,
                                       SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE ); 

  // Convert destinationHandle to a Destination pointer
  destination = (Destination*)getHandledObject(destinationHandle.handle,
                                               DESTINATION_OBJECT);
  CNDCHK( destination == NULL, MQ_STATUS_INVALID_HANDLE ); 
  
  // Create the producer
  ERRCHK( session->createProducer(destination, &producer) );

  // Make the producer a valid handle
  producer->setIsExported(PR_TRUE);
  producerHandle->handle = producer->getHandle();

  releaseHandledObject(destination);
  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  ASSERT( producer == NULL );
  releaseHandledObject(destination);
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQCreateMessageProducer(const MQSessionHandle sessionHandle,
                          MQProducerHandle *    producerHandle)
{
  static const char FUNCNAME[] = "MQCreateMessageProducer";
  MQError errorCode = MQ_SUCCESS;
  MessageProducer * producer = NULL;
  Session * session = NULL;
  
  CLEAR_ERROR_TRACE(PR_FALSE);

  CNDCHK( producerHandle == NULL, MQ_NULL_PTR_ARG );
  producerHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Convert sessionHandle to a Session pointer
  session = (Session*)getHandledObject(sessionHandle.handle,
                                       SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE ); 
  
  // Create the producer
  ERRCHK( session->createProducer(&producer) );

  // Make the producer a valid handle
  producer->setIsExported(PR_TRUE);
  producerHandle->handle = producer->getHandle();

  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  ASSERT( producer == NULL );
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}

/*
 */
EXPORTED_SYMBOL MQStatus
MQCreateMessageConsumer(const MQSessionHandle     sessionHandle,
                        const MQDestinationHandle destinationHandle,
                        ConstMQString             messageSelector,
                        MQBool                    noLocal,
                        MQConsumerHandle *        consumerHandle)
{
  static const char FUNCNAME[] = "MQCreateMessageConsumer";
  MQError errorCode = MQ_SUCCESS;
  MessageConsumer * consumer = NULL;
  Session * session = NULL;
  Destination * destination = NULL;

  CLEAR_ERROR_TRACE( PR_FALSE );
    
  CNDCHK( consumerHandle == NULL, MQ_NULL_PTR_ARG );
  consumerHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Convert sessionHandle to a Session pointer
  session = (Session*)getHandledObject(sessionHandle.handle,
                                       SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE ); 
  CNDCHK( session->getReceiveMode() != SESSION_SYNC_RECEIVE, MQ_NOT_SYNC_RECEIVE_MODE );

  // Convert destinationHandle to a Destination pointer
  destination = (Destination*)getHandledObject(destinationHandle.handle,
                                               DESTINATION_OBJECT);
  CNDCHK( destination == NULL, MQ_STATUS_INVALID_HANDLE ); 
  
  // Create the consumer
  if (messageSelector != NULL) {
    UTF8String messageSelectorStr(messageSelector);
    CNDCHK( STRCMP( messageSelectorStr.toString(), messageSelector ) != 0, MQ_OUT_OF_MEMORY );
    ERRCHK( session->createConsumer(destination, PR_FALSE, PR_FALSE, NULL, 
                                    &messageSelectorStr, noLocal, NULL, NULL, &consumer) );
  } else {
    ERRCHK( session->createConsumer(destination, PR_FALSE, PR_FALSE, NULL,
                                    NULL, noLocal, NULL, NULL, &consumer) );
  } 

  // Make the consumer a valid handle
  consumer->setIsExported(PR_TRUE);
  consumerHandle->handle = consumer->getHandle();

  releaseHandledObject(destination);
  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  ASSERT( consumer == NULL );
  releaseHandledObject(destination);
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQCreateSharedMessageConsumer(const MQSessionHandle     sessionHandle,
                        const MQDestinationHandle destinationHandle,
                        ConstMQString             subscriptionName,
                        ConstMQString             messageSelector,
                        MQConsumerHandle *        consumerHandle)
{
  static const char FUNCNAME[] = "MQCreateSharedMessageConsumer";
  MQError errorCode = MQ_SUCCESS;
  MessageConsumer * consumer = NULL;
  Session * session = NULL;
  Destination * destination = NULL;

  CLEAR_ERROR_TRACE( PR_FALSE );

  CNDCHK( subscriptionName == NULL, MQ_NULL_PTR_ARG );
    
  CNDCHK( consumerHandle == NULL, MQ_NULL_PTR_ARG );
  consumerHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Convert sessionHandle to a Session pointer
  session = (Session*)getHandledObject(sessionHandle.handle,
                                       SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE ); 
  CNDCHK( session->getReceiveMode() != SESSION_SYNC_RECEIVE, MQ_NOT_SYNC_RECEIVE_MODE );

  // Convert destinationHandle to a Destination pointer
  destination = (Destination*)getHandledObject(destinationHandle.handle,
                                               DESTINATION_OBJECT);
  CNDCHK( destination == NULL, MQ_STATUS_INVALID_HANDLE ); 

  {
  UTF8String subscriptionNameStr(subscriptionName);
  CNDCHK( STRCMP( subscriptionNameStr.toString(), subscriptionName ) != 0, MQ_OUT_OF_MEMORY );
  
  // Create the consumer
  if (messageSelector != NULL) {
    UTF8String messageSelectorStr(messageSelector);
    CNDCHK( STRCMP( messageSelectorStr.toString(), messageSelector ) != 0, MQ_OUT_OF_MEMORY );
    ERRCHK( session->createConsumer(destination, PR_FALSE, PR_TRUE, &subscriptionNameStr,
                                    &messageSelectorStr, PR_FALSE, NULL, NULL, &consumer) );
  } else {
    ERRCHK( session->createConsumer(destination, PR_FALSE, PR_TRUE, &subscriptionNameStr,
                                    NULL, PR_FALSE, NULL, NULL, &consumer) );
  } 
  }

  // Make the consumer a valid handle
  consumer->setIsExported(PR_TRUE);
  consumerHandle->handle = consumer->getHandle();

  releaseHandledObject(destination);
  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  ASSERT( consumer == NULL );
  releaseHandledObject(destination);
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQCreateAsyncMessageConsumer(const MQSessionHandle     sessionHandle,
                             const MQDestinationHandle destinationHandle,
                             ConstMQString             messageSelector,
                             MQBool                    noLocal,
                             MQMessageListenerFunc     messageListener,
                             void *                    messageListenerCallbackData,
                             MQConsumerHandle *        consumerHandle)
{

  static const char FUNCNAME[] = "MQCreateAsyncMessageConsumer";
  MQError errorCode = MQ_SUCCESS;
  MessageConsumer * consumer = NULL;
  Session * session = NULL;
  Destination * destination = NULL;
 
  CLEAR_ERROR_TRACE(PR_FALSE);

  CNDCHK( consumerHandle == NULL, MQ_NULL_PTR_ARG );
  CNDCHK( messageListener == NULL, MQ_NULL_PTR_ARG );
  consumerHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Convert sessionHandle to a Session pointer
  session = (Session*)getHandledObject(sessionHandle.handle,
                                       SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE );
  CNDCHK( session->getReceiveMode() != SESSION_ASYNC_RECEIVE, MQ_NOT_ASYNC_RECEIVE_MODE );

  // Convert destinationHandle to a Destination pointer
  destination = (Destination*)getHandledObject(destinationHandle.handle,
                                               DESTINATION_OBJECT);
  CNDCHK( destination == NULL, MQ_STATUS_INVALID_HANDLE );

  // Create the consumer
  if (messageSelector != NULL) {
    UTF8String messageSelectorStr(messageSelector);
    CNDCHK( STRCMP( messageSelectorStr.toString(), messageSelector ) != 0, MQ_OUT_OF_MEMORY );
    ERRCHK( session->createConsumer(destination, PR_FALSE, PR_FALSE, NULL,
                                    &messageSelectorStr, noLocal, messageListener,
                                    messageListenerCallbackData, &consumer) );
  } else {
    ERRCHK( session->createConsumer(destination, PR_FALSE, PR_FALSE, NULL,
                                    NULL, noLocal, messageListener, 
                                    messageListenerCallbackData, &consumer) );
  }

  // Make the consumer a valid handle
  consumer->setIsExported(PR_TRUE);
  consumerHandle->handle = consumer->getHandle();

  releaseHandledObject(destination);
  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  ASSERT( consumer == NULL );
  releaseHandledObject(destination);
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );

}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQCreateAsyncSharedMessageConsumer(const MQSessionHandle     sessionHandle,
                             const MQDestinationHandle destinationHandle,
                             ConstMQString             subscriptionName,
                             ConstMQString             messageSelector,
                             MQMessageListenerFunc     messageListener,
                             void *                    messageListenerCallbackData,
                             MQConsumerHandle *        consumerHandle)
{

  static const char FUNCNAME[] = "MQCreateAsyncSharedMessageConsumer";
  MQError errorCode = MQ_SUCCESS;
  MessageConsumer * consumer = NULL;
  Session * session = NULL;
  Destination * destination = NULL;
 
  CLEAR_ERROR_TRACE(PR_FALSE);

  CNDCHK( subscriptionName == NULL, MQ_NULL_PTR_ARG );

  CNDCHK( consumerHandle == NULL, MQ_NULL_PTR_ARG );
  CNDCHK( messageListener == NULL, MQ_NULL_PTR_ARG );
  consumerHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Convert sessionHandle to a Session pointer
  session = (Session*)getHandledObject(sessionHandle.handle,
                                       SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE );
  CNDCHK( session->getReceiveMode() != SESSION_ASYNC_RECEIVE, MQ_NOT_ASYNC_RECEIVE_MODE );

  // Convert destinationHandle to a Destination pointer
  destination = (Destination*)getHandledObject(destinationHandle.handle,
                                               DESTINATION_OBJECT);
  CNDCHK( destination == NULL, MQ_STATUS_INVALID_HANDLE );

  {
  UTF8String subscriptionNameStr(subscriptionName);
  CNDCHK( STRCMP( subscriptionNameStr.toString(), subscriptionName ) != 0, MQ_OUT_OF_MEMORY );

  // Create the consumer
  if (messageSelector != NULL) {
    UTF8String messageSelectorStr(messageSelector);
    CNDCHK( STRCMP( messageSelectorStr.toString(), messageSelector ) != 0, MQ_OUT_OF_MEMORY );
    ERRCHK( session->createConsumer(destination, PR_FALSE, PR_TRUE, &subscriptionNameStr,
                                    &messageSelectorStr, PR_FALSE, messageListener,
                                    messageListenerCallbackData, &consumer) );
  } else {
    ERRCHK( session->createConsumer(destination, PR_FALSE, PR_TRUE, &subscriptionNameStr,
                                    NULL, PR_FALSE, messageListener, 
                                    messageListenerCallbackData, &consumer) );
  }
  }

  // Make the consumer a valid handle
  consumer->setIsExported(PR_TRUE);
  consumerHandle->handle = consumer->getHandle();

  releaseHandledObject(destination);
  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  ASSERT( consumer == NULL );
  releaseHandledObject(destination);
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQCreateDurableMessageConsumer(const MQSessionHandle     sessionHandle,
                               const MQDestinationHandle destinationHandle,
                               ConstMQString             durableName,
                               ConstMQString             messageSelector,
                               MQBool                    noLocal, 
                               MQConsumerHandle *        consumerHandle)
{
  static const char FUNCNAME[] = "MQCreateDurableMessageConsumer";
  MQError errorCode = MQ_SUCCESS;
  MessageConsumer * consumer = NULL;
  Destination * destination = NULL;
  Session * session = NULL;

  CLEAR_ERROR_TRACE( PR_FALSE );
  
  CNDCHK( consumerHandle == NULL, MQ_NULL_PTR_ARG );
  consumerHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  CNDCHK( durableName == NULL, MQ_NULL_PTR_ARG );

  // Convert sessionHandle to a Session pointer
  session = (Session*)getHandledObject(sessionHandle.handle,
                                       SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE ); 
  CNDCHK( session->getReceiveMode() != SESSION_SYNC_RECEIVE, MQ_NOT_SYNC_RECEIVE_MODE );

  // Convert destinationHandle to a Destination pointer
  destination = (Destination*)getHandledObject(destinationHandle.handle,
                                               DESTINATION_OBJECT);
  CNDCHK( destination == NULL, MQ_STATUS_INVALID_HANDLE ); 
  
  // Create the consumer
  {
    UTF8String durableNameStr(durableName);
    CNDCHK( STRCMP( durableNameStr.toString(), durableName ) != 0, MQ_OUT_OF_MEMORY );

    if (messageSelector != NULL) {
      UTF8String messageSelectorStr(messageSelector);
      CNDCHK( STRCMP( messageSelectorStr.toString(), messageSelector ) != 0, MQ_OUT_OF_MEMORY );
      ERRCHK( session->createConsumer(destination, PR_TRUE, PR_FALSE, &durableNameStr, 
                                      &messageSelectorStr, noLocal, NULL, NULL, &consumer) );
    } else {
      ERRCHK( session->createConsumer(destination, PR_TRUE, PR_FALSE, &durableNameStr, 
                                      NULL, noLocal, NULL, NULL, &consumer) );
    }
  }

  // session->createConsumer should enforce this 
  ASSERT( !destination->getIsQueue() );

  // Make the consumer a valid handle
  consumer->setIsExported(PR_TRUE);
  consumerHandle->handle = consumer->getHandle();

  releaseHandledObject(destination);
  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  ASSERT( consumer == NULL );
  releaseHandledObject(destination);
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQCreateSharedDurableMessageConsumer(const MQSessionHandle     sessionHandle,
                               const MQDestinationHandle destinationHandle,
                               ConstMQString             durableName,
                               ConstMQString             messageSelector,
                               MQConsumerHandle *        consumerHandle)
{
  static const char FUNCNAME[] = "MQCreateSharedDurableMessageConsumer";
  MQError errorCode = MQ_SUCCESS;
  MessageConsumer * consumer = NULL;
  Destination * destination = NULL;
  Session * session = NULL;

  CLEAR_ERROR_TRACE( PR_FALSE );
  
  CNDCHK( consumerHandle == NULL, MQ_NULL_PTR_ARG );
  consumerHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  CNDCHK( durableName == NULL, MQ_NULL_PTR_ARG );

  // Convert sessionHandle to a Session pointer
  session = (Session*)getHandledObject(sessionHandle.handle,
                                       SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE ); 
  CNDCHK( session->getReceiveMode() != SESSION_SYNC_RECEIVE, MQ_NOT_SYNC_RECEIVE_MODE );

  // Convert destinationHandle to a Destination pointer
  destination = (Destination*)getHandledObject(destinationHandle.handle,
                                               DESTINATION_OBJECT);
  CNDCHK( destination == NULL, MQ_STATUS_INVALID_HANDLE ); 
  
  // Create the consumer
  {
    UTF8String durableNameStr(durableName);
    CNDCHK( STRCMP( durableNameStr.toString(), durableName ) != 0, MQ_OUT_OF_MEMORY );

    if (messageSelector != NULL) {
      UTF8String messageSelectorStr(messageSelector);
      CNDCHK( STRCMP( messageSelectorStr.toString(), messageSelector ) != 0, MQ_OUT_OF_MEMORY );
      ERRCHK( session->createConsumer(destination, PR_TRUE, PR_TRUE, &durableNameStr, 
                                      &messageSelectorStr, PR_FALSE, NULL, NULL, &consumer) );
    } else {
      ERRCHK( session->createConsumer(destination, PR_TRUE, PR_TRUE, &durableNameStr, 
                                      NULL, PR_FALSE, NULL, NULL, &consumer) );
    }
  }

  // session->createConsumer should enforce this 
  ASSERT( !destination->getIsQueue() );

  // Make the consumer a valid handle
  consumer->setIsExported(PR_TRUE);
  consumerHandle->handle = consumer->getHandle();

  releaseHandledObject(destination);
  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  ASSERT( consumer == NULL );
  releaseHandledObject(destination);
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQCreateAsyncDurableMessageConsumer(const MQSessionHandle     sessionHandle,
                                    const MQDestinationHandle destinationHandle,
                                    ConstMQString             durableName,
                                    ConstMQString             messageSelector,
                                    MQBool                    noLocal,
                                    MQMessageListenerFunc     messageListener,
                                    void *                    messageListenerCallbackData,
                                    MQConsumerHandle *        consumerHandle)
{
  static const char FUNCNAME[] = "MQCreateAsyncDurableMessageConsumer";
  MQError errorCode = MQ_SUCCESS;
  MessageConsumer * consumer = NULL;
  Destination * destination = NULL;
  Session * session = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);
 
  CNDCHK( consumerHandle == NULL, MQ_NULL_PTR_ARG );
  CNDCHK( messageListener == NULL, MQ_NULL_PTR_ARG );
  consumerHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  CNDCHK( durableName == NULL, MQ_NULL_PTR_ARG );

  // Convert sessionHandle to a Session pointer
  session = (Session*)getHandledObject(sessionHandle.handle,
                                       SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE );
  CNDCHK( session->getReceiveMode() != SESSION_ASYNC_RECEIVE, MQ_NOT_ASYNC_RECEIVE_MODE );

  // Convert destinationHandle to a Destination pointer
  destination = (Destination*)getHandledObject(destinationHandle.handle,
                                               DESTINATION_OBJECT);
  CNDCHK( destination == NULL, MQ_STATUS_INVALID_HANDLE );
 
  // Create the consumer
  {
    UTF8String durableNameStr(durableName);
    CNDCHK( STRCMP( durableNameStr.toString(), durableName ) != 0, MQ_OUT_OF_MEMORY );

    if (messageSelector != NULL) {
      UTF8String messageSelectorStr(messageSelector);
      CNDCHK( STRCMP( messageSelectorStr.toString(), messageSelector ) != 0, MQ_OUT_OF_MEMORY );
      ERRCHK( session->createConsumer(destination, PR_TRUE, PR_FALSE, &durableNameStr,
                                      &messageSelectorStr, noLocal, messageListener,
                                      messageListenerCallbackData, &consumer) );
    } else {
      ERRCHK( session->createConsumer(destination, PR_TRUE, PR_FALSE, &durableNameStr,
                                      NULL, noLocal, messageListener,
                                      messageListenerCallbackData, &consumer) );
    }
  }

  // session->createConsumer should enforce this
  ASSERT( !destination->getIsQueue() );

  // Make the consumer a valid handle
  consumer->setIsExported(PR_TRUE);
  consumerHandle->handle = consumer->getHandle();

  releaseHandledObject(destination);
  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  ASSERT( consumer == NULL );
  releaseHandledObject(destination);
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );

}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQCreateAsyncSharedDurableMessageConsumer(const MQSessionHandle     sessionHandle,
                                    const MQDestinationHandle destinationHandle,
                                    ConstMQString             durableName,
                                    ConstMQString             messageSelector,
                                    MQMessageListenerFunc     messageListener,
                                    void *                    messageListenerCallbackData,
                                    MQConsumerHandle *        consumerHandle)
{
  static const char FUNCNAME[] = "MQCreateAsyncSharedDurableMessageConsumer";
  MQError errorCode = MQ_SUCCESS;
  MessageConsumer * consumer = NULL;
  Destination * destination = NULL;
  Session * session = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);
 
  CNDCHK( consumerHandle == NULL, MQ_NULL_PTR_ARG );
  CNDCHK( messageListener == NULL, MQ_NULL_PTR_ARG );
  consumerHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  CNDCHK( durableName == NULL, MQ_NULL_PTR_ARG );

  // Convert sessionHandle to a Session pointer
  session = (Session*)getHandledObject(sessionHandle.handle,
                                       SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE );
  CNDCHK( session->getReceiveMode() != SESSION_ASYNC_RECEIVE, MQ_NOT_ASYNC_RECEIVE_MODE );

  // Convert destinationHandle to a Destination pointer
  destination = (Destination*)getHandledObject(destinationHandle.handle,
                                               DESTINATION_OBJECT);
  CNDCHK( destination == NULL, MQ_STATUS_INVALID_HANDLE );
 
  // Create the consumer
  {
    UTF8String durableNameStr(durableName);
    CNDCHK( STRCMP( durableNameStr.toString(), durableName ) != 0, MQ_OUT_OF_MEMORY );

    if (messageSelector != NULL) {
      UTF8String messageSelectorStr(messageSelector);
      CNDCHK( STRCMP( messageSelectorStr.toString(), messageSelector ) != 0, MQ_OUT_OF_MEMORY );
      ERRCHK( session->createConsumer(destination, PR_TRUE, PR_TRUE, &durableNameStr,
                                      &messageSelectorStr, PR_FALSE, messageListener,
                                      messageListenerCallbackData, &consumer) );
    } else {
      ERRCHK( session->createConsumer(destination, PR_TRUE, PR_TRUE, &durableNameStr,
                                      NULL, PR_FALSE, messageListener,
                                      messageListenerCallbackData, &consumer) );
    }
  }

  // session->createConsumer should enforce this
  ASSERT( !destination->getIsQueue() );

  // Make the consumer a valid handle
  consumer->setIsExported(PR_TRUE);
  consumerHandle->handle = consumer->getHandle();

  releaseHandledObject(destination);
  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  ASSERT( consumer == NULL );
  releaseHandledObject(destination);
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );

}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQUnsubscribeDurableMessageConsumer(const MQSessionHandle sessionHandle,
                                      ConstMQString const durableName)
{
  static const char FUNCNAME[] = "MQUnsubscribeDurableMessageConsumer";
  MQError errorCode = MQ_SUCCESS;
  Session * session = NULL;

  CLEAR_ERROR_TRACE( PR_FALSE );
  
  CNDCHK( durableName == NULL, MQ_NULL_PTR_ARG );

  // Convert sessionHandle to a Session pointer
  session = (Session*)getHandledObject(sessionHandle.handle,
                                                 SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE ); 
  
  // Unsubscribe the consumer
  {
    UTF8String durableNameStr(durableName);
    CNDCHK( STRCMP( durableNameStr.toString(), durableName ) != 0, MQ_OUT_OF_MEMORY );
    ERRCHK( session->unsubscribeDurableConsumer(&durableNameStr) );
  }

  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}


EXPORTED_SYMBOL MQStatus
MQRecoverSession(const MQSessionHandle sessionHandle)
{
  static const char FUNCNAME[] = "MQRecoverSession";
  MQError errorCode = MQ_SUCCESS;
  Session * session = NULL;
  
  CLEAR_ERROR_TRACE(PR_FALSE);

  session = (Session*)getHandledObject(sessionHandle.handle,
                                                 SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE ); 
  CNDCHK( session->getAckMode() == SESSION_TRANSACTED, MQ_TRANSACTED_SESSION );
  
  ERRCHK( session->recover(PR_FALSE) );

  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}


EXPORTED_SYMBOL MQStatus
MQCommitSession(const MQSessionHandle sessionHandle)
{
  static const char FUNCNAME[] = "MQCommitSession";
  MQError errorCode = MQ_SUCCESS;
  Session * session = NULL;

  CLEAR_ERROR_TRACE( PR_FALSE );
  
  session = (Session*)getHandledObject(sessionHandle.handle,
                                                 SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE ); 
  CNDCHK( session->getAckMode() != SESSION_TRANSACTED, MQ_NOT_TRANSACTED_SESSION );
  
  ERRCHK( session->commit() );

  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}


EXPORTED_SYMBOL MQStatus
MQRollbackSession(const MQSessionHandle sessionHandle)
{
  static const char FUNCNAME[] = "MQRollbackSession";
  MQError errorCode = MQ_SUCCESS;
  Session * session = NULL;
  
  CLEAR_ERROR_TRACE(PR_FALSE);

  session = (Session*)getHandledObject(sessionHandle.handle,
                                                 SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE ); 
  CNDCHK( session->getAckMode() != SESSION_TRANSACTED, MQ_NOT_TRANSACTED_SESSION );
  
  ERRCHK( session->rollback() );

  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}


EXPORTED_SYMBOL MQStatus
MQGetAcknowledgeMode(const MQSessionHandle sessionHandle, MQAckMode * mqAckMode)
{
  static const char FUNCNAME[] = "MQGetAcknowledgeMode";
  MQError errorCode = MQ_SUCCESS;
  Session * session = NULL;
  AckMode ackMode;

  CLEAR_ERROR_TRACE(PR_FALSE);
 
  NULLCHK( mqAckMode );
  session = (Session*)getHandledObject(sessionHandle.handle,
                                                 SESSION_OBJECT);
  CNDCHK( session == NULL, MQ_STATUS_INVALID_HANDLE );
  ackMode = session->getAckMode(); 

  switch(ackMode) {
  case AUTO_ACKNOWLEDGE:
    *mqAckMode = MQ_AUTO_ACKNOWLEDGE;
    break;
  case CLIENT_ACKNOWLEDGE:
    *mqAckMode = MQ_CLIENT_ACKNOWLEDGE;
    break;
  case DUPS_OK_ACKNOWLEDGE:
    *mqAckMode = MQ_DUPS_OK_ACKNOWLEDGE;
    break;
  case SESSION_TRANSACTED:
    *mqAckMode = MQ_SESSION_TRANSACTED;
    break;
  default:
    ASSERT( MQ_FALSE );
    ERRCHK( MQ_INVALID_ACKNOWLEDGE_MODE );
  }

  releaseHandledObject(session);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(session);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);
  RETURN_STATUS( errorCode );
}

