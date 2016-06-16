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
 * @(#)MessageProducer.cpp	1.9 06/26/07
 */ 

#include "MessageProducer.hpp"
#include "../util/UtilityMacros.h"
#include "Session.hpp"

/*
 *
 */
MessageProducer::MessageProducer(Session *     const sessionArg)
{
  CHECK_OBJECT_VALIDITY();

  ASSERT( sessionArg != NULL );
  init();
  this->session = sessionArg;
  this->destination = NULL;
}


/*
 *
 */
MessageProducer::MessageProducer(Session *     const sessionArg, 
                                 Destination * const destinationArg)
{
  CHECK_OBJECT_VALIDITY();

  ASSERT( sessionArg != NULL );
  ASSERT( destinationArg != NULL );

  init();
  this->session = sessionArg;
  this->destination = destinationArg->clone();
  if (this->destination == NULL) {
     this->initializationError = MQ_OUT_OF_MEMORY; 
  }
}

/*
 *
 */
void
MessageProducer::init()
{
  CHECK_OBJECT_VALIDITY();

  isClosed = PR_FALSE;
  session = NULL;
  destination = NULL;
  deliveryMode = MESSAGE_PRODUCER_DEFAULT_DELIVERY_MODE;
  priority = MESSAGE_PRODUCER_DEFAULT_PRIORITY;
  timeToLive = MESSAGE_PRODUCER_DEFAULT_TIME_TO_LIVE;
  deliveryDelay = MESSAGE_PRODUCER_DEFAULT_DELIVERY_DELAY;
  this->initializationError = MQ_SUCCESS;
}

/*
 *
 */
MQError
MessageProducer::getInitializationError() const
{
  CHECK_OBJECT_VALIDITY();
  RETURN_IF_ERROR( HandledObject::getInitializationError() );

  return initializationError;
}

/*
 *
 */
MessageProducer::~MessageProducer()
{
  CHECK_OBJECT_VALIDITY();

  this->close();
  this->session = NULL;
  HANDLED_DELETE( this->destination );
}


/*
 *
 */
iMQError 
MessageProducer::close()
{
  CHECK_OBJECT_VALIDITY();
  MQError errorCode = MQ_SUCCESS;
  UTF8String * destKey = NULL;
  Long * producerIDLong = NULL;

  monitor.enter();

  if (this->isClosed == PR_TRUE) {
    monitor.exit();
    return MQ_SUCCESS;
  }
  this->isClosed = PR_TRUE;

  ERRCHK( validatedDestinations.keyIterationStart() );
  while (validatedDestinations.keyIterationHasNext()) {
    ERRCHK( validatedDestinations.keyIterationGetNext((const BasicType ** const)&destKey) );
    ERRCHK( validatedDestinations.getValueFromKey(destKey, (const Object ** const)&producerIDLong) );
    NULLCHK( producerIDLong );
    ERRCHK( this->session->unregisterMessageProducer(producerIDLong->getValue()) );
  }
  
Cleanup:
  monitor.exit();
  return errorCode;
}



/*
 *
 */
Session *
MessageProducer::getSession() const
{
  CHECK_OBJECT_VALIDITY();

  return this->session;
}


/*
 *
 */
const Destination * 
MessageProducer::getDestination() const
{
  CHECK_OBJECT_VALIDITY();

  return this->destination;
}



/*
 *
 */
iMQError 
MessageProducer::writeJMSMessage(Message * const message)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( message );

  return writeJMSMessage(message,
                         this->getDestination(),
                         this->getDeliveryMode(),
                         (PRInt8)this->getPriority(),
                         this->getTimeToLive());
}

/*
 *
 */
iMQError 
MessageProducer::writeJMSMessage(Message * const message,
                                 const Destination * const msgDestination)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( message );
  RETURN_ERROR_IF_NULL( msgDestination );
  RETURN_ERROR_IF( this->destination != NULL, MQ_PRODUCER_HAS_DESTINATION);

  return this->writeJMSMessage(message,
                               msgDestination,
                               this->getDeliveryMode(),
                               (PRInt8)this->getPriority(),
                               this->getTimeToLive());
}


/*
 *
 */
iMQError 
MessageProducer::writeJMSMessage(Message * const message,
                                 const PRInt32 msgDeliveryMode,
                                 const PRInt8  msgPriority,
                                 const PRInt64 msgTimeToLive)
{
  return this->writeJMSMessage(message, 
                               this->getDestination(),
                               msgDeliveryMode,
                               msgPriority,
                               msgTimeToLive);
}

/*
 *
 */
iMQError 
MessageProducer::writeJMSMessage(Message * const message,
                                 const Destination * const msgDestination,
                                 const PRInt32 msgDeliveryMode,
                                 const PRInt8  msgPriority,
                                 const PRInt64 msgTimeToLive)
{
  static const char FUNCNAME[] = "writeJMSMessage";
  CHECK_OBJECT_VALIDITY();
  MQError errorCode = MQ_SUCCESS;
  PRInt64 producerID = 0;

  RETURN_ERROR_IF_NULL( message );
  RETURN_ERROR_IF_NULL( msgDestination );

  RETURN_IF_ERROR( message->setJMSDestination(msgDestination) );
  RETURN_IF_ERROR( message->setJMSDeliveryMode(msgDeliveryMode) );

  CNDCHK( msgPriority < 0 || msgPriority > 9, MQ_INVALID_PRIORITY );
  RETURN_IF_ERROR( message->setJMSPriority(msgPriority) );

  RETURN_IF_ERROR( message->setJMSExpiration(msgTimeToLive) );
  RETURN_IF_ERROR( message->setJMSDeliveryTime(this->getDeliveryDelay()) );

  // Make sure this producer can send a message to this destination
  ERRCHK( this->validateDestination(msgDestination, &producerID) );

  ERRCHK( this->session->writeJMSMessage(message, producerID) );

  return MQ_SUCCESS;

Cleanup:
  MQ_ERROR_TRACE( FUNCNAME, errorCode ); 
  return errorCode;
}


/*
 *
 */
iMQError 
MessageProducer::validateDestination(const Destination * const msgDestination, PRInt64 * producerID)
{
  CHECK_OBJECT_VALIDITY();
  iMQError errorCode = IMQ_SUCCESS;
  const UTF8String * destinationName = NULL;
  UTF8String * destKey = NULL;
  Long * producerIDLong = NULL;
  PRInt64 id = 0;

  NULLCHK( msgDestination );

  destinationName = msgDestination->getName();
  CNDCHK( destinationName == NULL, IMQ_DESTINATION_NO_NAME );
                   
  // If we have already checked this destination, don't check it again
  if (validatedDestinations.getValueFromKey(destinationName,
                              (const Object **)&producerIDLong) == IMQ_SUCCESS)
  {
    if (producerID != NULL) *producerID = producerIDLong->getValue();
    return IMQ_SUCCESS;
  }

  // Return an error if this producer cannot send to msgDestination
  RETURN_IF_ERROR( session->registerMessageProducer(msgDestination, &id) );

  // Add msgDestination to the list of valid destinations
  MEMCHK( destKey = (UTF8String*)destinationName->clone() );
  MEMCHK( producerIDLong = new Long(id) );

  monitor.enter();
  if (this->isClosed == PR_TRUE) {
    errorCode = MQ_PRODUCER_CLOSED;
  } else {
    errorCode = validatedDestinations.addEntry(destKey, producerIDLong);
  }
  monitor.exit();
  ERRCHK( errorCode );
  destKey = NULL;
  producerIDLong = NULL;
  if (producerID != NULL) *producerID = id;

  return IMQ_SUCCESS;

Cleanup:
  if (LL_IS_ZERO(id) == 0) {
    session->unregisterMessageProducer(id);
  }
  DELETE( destKey );
  DELETE( producerIDLong );
  return errorCode;
}


/*
 *
 */
PRInt32
MessageProducer::getDeliveryMode() const
{
  CHECK_OBJECT_VALIDITY();

  return this->deliveryMode;
}

/*
 *
 */
PRInt32
MessageProducer::getPriority() const
{
  CHECK_OBJECT_VALIDITY();

  return this->priority;
}

/*
 *
 */
PRInt64
MessageProducer::getTimeToLive() const
{
  CHECK_OBJECT_VALIDITY();

  return this->timeToLive;
}

/*
 *
 */
void 
MessageProducer::setDeliveryMode(const PRInt32 deliveryModeArg)
{
  CHECK_OBJECT_VALIDITY();

  this->deliveryMode = deliveryModeArg;
}

/*
 *
 */
void 
MessageProducer::setPriority(const PRInt32 priorityArg)
{
  CHECK_OBJECT_VALIDITY();

  ASSERT( (priorityArg >= MESSAGE_PRODUCER_MIN_PRIORITY) && 
          (priorityArg <= MESSAGE_PRODUCER_MAX_PRIORITY) );
  if (priorityArg < MESSAGE_PRODUCER_MIN_PRIORITY) {
    this->priority = MESSAGE_PRODUCER_MIN_PRIORITY;
  } else if (priorityArg > MESSAGE_PRODUCER_MAX_PRIORITY) {
    this->priority = MESSAGE_PRODUCER_MAX_PRIORITY;
  } else {
    this->priority = priorityArg;
  }
}

/*
 *
 */
void 
MessageProducer::setTimeToLive(const PRInt64 timeToLiveArg)
{
  CHECK_OBJECT_VALIDITY();

  this->timeToLive = timeToLiveArg;
}

/*
 */
void 
MessageProducer::setDeliveryDelay(const PRInt64 deliveryDelayArg)
{
  CHECK_OBJECT_VALIDITY();

  this->deliveryDelay = deliveryDelayArg;
}

PRInt64
MessageProducer::getDeliveryDelay() const
{
  CHECK_OBJECT_VALIDITY();

  return this->deliveryDelay;
}


/*
 *
 */
HandledObjectType
MessageProducer::getObjectType() const
{
  CHECK_OBJECT_VALIDITY();

  return MESSAGE_PRODUCER_OBJECT;
}






/*
 *
 */
iMQError 
MessageProducer::send(Message * const message, const Destination * const msgDestination)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( message );
  RETURN_ERROR_IF_NULL( msgDestination );
  RETURN_ERROR_IF( this->destination != NULL, MQ_PRODUCER_HAS_DESTINATION);
  
  RETURN_IF_ERROR( this->writeJMSMessage(message, msgDestination) );

  return IMQ_SUCCESS;
}

/*
 *
 */
iMQError 
MessageProducer::send(Message * const message)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( message );
  RETURN_ERROR_IF( this->destination == NULL, IMQ_PRODUCER_NO_DESTINATION );
  
  RETURN_IF_ERROR( this->writeJMSMessage(message) );

  return IMQ_SUCCESS;
}



/*
 *
 */
iMQError 
MessageProducer::send(Message * const message,
                      const Destination * const msgDestination,
                      const PRInt32 msgDeliveryMode,
                      const PRInt8  msgPriority,
                      const PRInt64 msgTimeToLive)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( message );
  RETURN_ERROR_IF_NULL( msgDestination );

  RETURN_ERROR_IF( this->destination != NULL, MQ_PRODUCER_HAS_DESTINATION);

  return this->writeJMSMessage(message, msgDestination, msgDeliveryMode, 
                               msgPriority, msgTimeToLive);
}



/*
 *
 */
iMQError 
MessageProducer::send(Message * const message,
                      const PRInt32 msgDeliveryMode,
                      const PRInt8  msgPriority,
                      const PRInt64 msgTimeToLive)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( message );
  RETURN_ERROR_IF( this->destination == NULL, IMQ_PRODUCER_NO_DESTINATION );

  return this->writeJMSMessage(message, msgDeliveryMode, msgPriority, 
                               msgTimeToLive);
}


