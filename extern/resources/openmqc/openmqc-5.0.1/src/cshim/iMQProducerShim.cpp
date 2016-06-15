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
 * @(#)iMQProducerShim.cpp	1.12 06/26/07
 */ 

#include "mqproducer.h"
#include "shimUtils.hpp"
#include "../client/MessageProducer.hpp"
#include "../client/Message.hpp"
#include "../client/Session.hpp"



/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQCloseMessageProducer(MQProducerHandle producerHandle)
{
  static const char FUNCNAME[] = "MQCloseMessageProducer";
  MQError errorCode = MQ_SUCCESS;
  Session * session = NULL;
  MessageProducer * producer = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);
                                                                  
  // Convert producerHandle to a MessageProducer pointer
  producer = (MessageProducer*)getHandledObject(producerHandle.handle, 
                                                MESSAGE_PRODUCER_OBJECT);
  CNDCHK( producer == NULL, MQ_STATUS_INVALID_HANDLE);

  // Close the producer via the session
  session = producer->getSession();
  if (session != NULL) {
    // This won't actually delete the producer because this function
    // still owns a pointer to it.
    ERRCHK( session->closeProducer(producer) );
  }

  // Release our pointer to the object, this actually deletes the consumer
  releaseHandledObject(producer);

  // This only has an effect if consumer->getSession() was NULL, which
  // should never happen.
  // freeHandledObject(producerHandle.handle, MESSAGE_PRODUCER_OBJECT);

  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(producer);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQSendMessage(const MQProducerHandle producerHandle,
                const MQMessageHandle messageHandle)
{
  static const char FUNCNAME[] = "MQSendMessage";
  MQError errorCode = MQ_SUCCESS;
  MessageProducer * producer = NULL;
  Message * message = NULL;
                                                                  
  CLEAR_ERROR_TRACE(PR_FALSE);

  // Convert producerHandle to a MessageProducer pointer
  producer = (MessageProducer*)getHandledObject(producerHandle.handle, 
                                                MESSAGE_PRODUCER_OBJECT);
  CNDCHK( producer == NULL, MQ_STATUS_INVALID_HANDLE);

  // Convert messageHandle to a Message pointer
  message = (Message*)getHandledObject(messageHandle.handle,
                                       MESSAGE_OBJECT);
  CNDCHK( message == NULL, MQ_STATUS_INVALID_HANDLE );

  // Send the message
  ERRCHK( producer->send(message) );

  releaseHandledObject(message);
  releaseHandledObject(producer);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(message);
  releaseHandledObject(producer);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQSendMessageExt(const MQProducerHandle producerHandle,
                  const MQMessageHandle messageHandle,
                  MQDeliveryMode msgDeliveryMode,
                  MQInt8 msgPriority,
                  MQInt64 msgTimeToLive)
{
  static const char FUNCNAME[] = "MQSendMessageExt";
  MQError errorCode = MQ_SUCCESS;
  MessageProducer * producer = NULL;
  Message * message = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);
                                                                  
  // Convert producerHandle to a MessageProducer pointer
  producer = (MessageProducer*)getHandledObject(producerHandle.handle, 
                                                MESSAGE_PRODUCER_OBJECT);
  CNDCHK( producer == NULL, MQ_STATUS_INVALID_HANDLE);

  // Convert messageHandle to a Message pointer
  message = (Message*)getHandledObject(messageHandle.handle,
                                       MESSAGE_OBJECT);
  CNDCHK( message == NULL, MQ_STATUS_INVALID_HANDLE );

  // Send the message
  if (msgDeliveryMode == MQ_PERSISTENT_DELIVERY) {
    ERRCHK( producer->send(message, PERSISTENT_DELIVERY, 
                           msgPriority, msgTimeToLive) );
  } else if (msgDeliveryMode == MQ_NON_PERSISTENT_DELIVERY) {
    ERRCHK( producer->send(message, NON_PERSISTENT_DELIVERY, 
                           msgPriority, msgTimeToLive) );
  } else {
    ERRCHK( MQ_INVALID_DELIVERY_MODE );
  }

  releaseHandledObject(message);
  releaseHandledObject(producer);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(message);
  releaseHandledObject(producer);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQSendMessageToDestination(const MQProducerHandle producerHandle,
                  const MQMessageHandle messageHandle,
                  const MQDestinationHandle destinationHandle)
{
  static const char FUNCNAME[] = "MQSendMessageToDestination";
  MQError errorCode = MQ_SUCCESS;
  MessageProducer * producer = NULL;
  Message * message = NULL;
  Destination * destination = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);
  
  // Convert producerHandle to a MessageProducer pointer
  producer = (MessageProducer*)getHandledObject(producerHandle.handle, 
                                                MESSAGE_PRODUCER_OBJECT);
  CNDCHK( producer == NULL, MQ_STATUS_INVALID_HANDLE);

  // Convert messageHandle to a Message pointer
  message = (Message*)getHandledObject(messageHandle.handle,
                                       MESSAGE_OBJECT);
  CNDCHK( message == NULL, MQ_STATUS_INVALID_HANDLE );

  // Convert destinationHandle to a Destination pointer
  destination = (Destination*)getHandledObject(destinationHandle.handle, 
                                               DESTINATION_OBJECT);
  CNDCHK( destination == NULL, MQ_STATUS_INVALID_HANDLE);

  // Send the message
  ERRCHK( producer->send(message, destination) );

  releaseHandledObject(destination);
  releaseHandledObject(message);
  releaseHandledObject(producer);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(destination);
  releaseHandledObject(message);
  releaseHandledObject(producer);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQSendMessageToDestinationExt(const MQProducerHandle producerHandle,
                    const MQMessageHandle messageHandle,
                    const MQDestinationHandle destinationHandle,
                    MQDeliveryMode msgDeliveryMode,
                    MQInt8 msgPriority,
                    MQInt64 msgTimeToLive)
{
  static const char FUNCNAME[] = "MQSendMessageToDestinationExt";
  MQError errorCode = MQ_SUCCESS;
  MessageProducer * producer = NULL;
  Message * message = NULL;
  Destination * destination = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);
  
  // Convert producerHandle to a MessageProducer pointer
  producer = (MessageProducer*)getHandledObject(producerHandle.handle, 
                                                MESSAGE_PRODUCER_OBJECT);
  CNDCHK( producer == NULL, MQ_STATUS_INVALID_HANDLE);

  // Convert messageHandle to a Message pointer
  message = (Message*)getHandledObject(messageHandle.handle,
                                       MESSAGE_OBJECT);
  CNDCHK( message == NULL, MQ_STATUS_INVALID_HANDLE );

  // Convert destinationHandle to a Destination pointer
  destination = (Destination*)getHandledObject(destinationHandle.handle, 
                                               DESTINATION_OBJECT);
  CNDCHK( destination == NULL, MQ_STATUS_INVALID_HANDLE);

  // Send the message
  if (msgDeliveryMode == MQ_PERSISTENT_DELIVERY) {
    ERRCHK( producer->send(message, destination, PERSISTENT_DELIVERY,
                           msgPriority, msgTimeToLive) );
  } else if (msgDeliveryMode == MQ_NON_PERSISTENT_DELIVERY) {
    ERRCHK( producer->send(message, destination, NON_PERSISTENT_DELIVERY,
                           msgPriority, msgTimeToLive) );
  } else {
    ERRCHK( MQ_INVALID_DELIVERY_MODE );
  }
  releaseHandledObject(destination);
  releaseHandledObject(message);
  releaseHandledObject(producer);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(destination);
  releaseHandledObject(message);
  releaseHandledObject(producer);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

EXPORTED_SYMBOL MQStatus 
MQSetDeliveryDelay(const MQProducerHandle producerHandle,
                   MQInt64 deliveryDelay)
{
  static const char FUNCNAME[] = "MQSetDeliveryDelay";
  MQError errorCode = MQ_SUCCESS;
  MessageProducer * producer = NULL;
 
  CLEAR_ERROR_TRACE(PR_FALSE);

  CNDCHK( LL_GE_ZERO(deliveryDelay) == 0, MQ_UNSUPPORTED_ARGUMENT_VALUE );

  // Convert producerHandle to a MessageProducer pointer
  producer = (MessageProducer*)getHandledObject(producerHandle.handle, 
                                                MESSAGE_PRODUCER_OBJECT);
  CNDCHK( producer == NULL, MQ_STATUS_INVALID_HANDLE);

  producer->setDeliveryDelay(deliveryDelay);

  releaseHandledObject(producer);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(producer);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}


EXPORTED_SYMBOL MQStatus 
MQGetDeliveryDelay(const MQProducerHandle producerHandle,
                   MQInt64 *deliveryDelay)
{
  static const char FUNCNAME[] = "MQGetDeliveryDelay";
  MQError errorCode = MQ_SUCCESS;
  MessageProducer * producer = NULL;
 
  CLEAR_ERROR_TRACE(PR_FALSE);

  NULLCHK( deliveryDelay );

  // Convert producerHandle to a MessageProducer pointer
  producer = (MessageProducer*)getHandledObject(producerHandle.handle, 
                                                MESSAGE_PRODUCER_OBJECT);
  CNDCHK( producer == NULL, MQ_STATUS_INVALID_HANDLE);

  *deliveryDelay = producer->getDeliveryDelay();

  releaseHandledObject(producer);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(producer);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

