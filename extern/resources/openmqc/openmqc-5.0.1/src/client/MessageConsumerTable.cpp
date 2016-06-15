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
 * @(#)MessageConsumerTable.cpp	1.6 06/26/07
 */ 

#include "MessageConsumerTable.hpp"
#include "MessageConsumer.hpp"
#include "Session.hpp"
#include "../basictypes/Long.hpp"
#include "../util/UtilityMacros.h"
#include "../util/LogUtils.hpp"

/*
 *
 */
MessageConsumerTable::MessageConsumerTable()
{
  CHECK_OBJECT_VALIDITY();

  PRBool autoDeleteKey, autoDeleteValue;
  this->table = new BasicTypeHashtable(autoDeleteKey=PR_TRUE, 
                                       autoDeleteValue=PR_FALSE); 
}

/*
 *
 */
MessageConsumerTable::~MessageConsumerTable()
{
  CHECK_OBJECT_VALIDITY();

  DELETE( this->table );
}

/*
 *
 */
MQError
MessageConsumerTable::remove(PRUint64 consumerIDArg, MessageConsumer ** const consumer)
{
  CHECK_OBJECT_VALIDITY();
  
  MQError errorCode = MQ_SUCCESS;
  MessageConsumer * mc = NULL;
  Long consumerIDLong(consumerIDArg);

  RETURN_ERROR_IF(this->table == NULL, MQ_OUT_OF_MEMORY);  

  monitor.enter();
  if (consumer != NULL) {
    errorCode = this->table->getValueFromKey(&consumerIDLong, (const Object** const)&mc);
    *consumer = mc;
    if (errorCode == MQ_SUCCESS) {
      errorCode = this->table->removeEntry(&consumerIDLong);
    }
  } else {
    errorCode = this->table->removeEntry(&consumerIDLong);
  }
  monitor.exit();

  if (errorCode != MQ_SUCCESS) {
    LOG_FINE(( CODELOC, MESSAGECONSUMERTABLE_LOG_MASK, NULL_CONN_ID, errorCode,
        "Failed to remove comsumerID=%lld from the MessageConsumerTable 0x%p because '%s' (%d)", 
         consumerIDArg, this, errorStr(errorCode), errorCode ));
  }

  return errorCode;
}

/*
 *
 */
MQError 
MessageConsumerTable::add(PRUint64 consumerIDArg, MessageConsumer * const consumer)
{
  CHECK_OBJECT_VALIDITY();

  MQError errorCode = MQ_SUCCESS;
  MessageConsumer * prevConsumer = NULL;
  Long * consumerIDLong = new Long(consumerIDArg);

  RETURN_ERROR_IF_NULL(consumer);

  RETURN_ERROR_IF(this->table == NULL, MQ_OUT_OF_MEMORY);
  RETURN_ERROR_IF(consumerIDLong == NULL, MQ_OUT_OF_MEMORY);

  monitor.enter();
  errorCode = this->table->getValueFromKey(consumerIDLong, (const Object** const)&prevConsumer);
  if (errorCode == MQ_SUCCESS) {
    LOG_FINE(( CODELOC, MESSAGECONSUMERTABLE_LOG_MASK, NULL_CONN_ID, errorCode,
        "MessageConsumerTable:comsumerID=%lld exists in MessageConsumerTable 0x%p",
         consumerIDArg, this ));
    errorCode = MQ_REUSED_CONSUMER_ID;
  } else if (errorCode == MQ_NOT_FOUND) {
    errorCode = this->table->addEntry(consumerIDLong, consumer);
  }
  monitor.exit();

  if (errorCode != MQ_SUCCESS) {
    DELETE( consumerIDLong );

    LOG_FINE(( CODELOC, MESSAGECONSUMERTABLE_LOG_MASK, NULL_CONN_ID, errorCode, 
        "Failed to add comsumerID=%lld to MessageConsumerTable 0x%p because '%s' (%d)",
         consumerIDArg, this, errorStr(errorCode), errorCode ));
  }

  return errorCode;
}


/*
 *
 */
MQError
MessageConsumerTable::get(PRUint64 consumerID, MessageConsumer ** const consumer)
{
  CHECK_OBJECT_VALIDITY();

  MQError errorCode = MQ_SUCCESS;
  Long consumerIDLong(consumerID);

  RETURN_ERROR_IF_NULL(consumer);

  RETURN_ERROR_IF(this->table == NULL, MQ_OUT_OF_MEMORY);

  monitor.enter();
  errorCode = this->table->getValueFromKey(&consumerIDLong, (const Object** const)consumer);
  monitor.exit();

  if (errorCode != MQ_SUCCESS) {
    LOG_FINE(( CODELOC, MESSAGECONSUMERTABLE_LOG_MASK, NULL_CONN_ID, errorCode,
        "Failed to get comsumerID=%lld from MessageConsumerTable 0x%p because '%s' (%d)",
         consumerID, this, errorStr(errorCode), errorCode ));
  }

  return errorCode;
}


/* 
 *
 */
MQError
MessageConsumerTable::operationAll(MessageConsumerOP op, const void * opData)
{
  CHECK_OBJECT_VALIDITY();

  MQError errorCode = MQ_SUCCESS;
  Long * consumerIDLong = NULL;
  MessageConsumer * consumer = NULL;

  // in case the 'new' in the constructor failed
  RETURN_ERROR_IF(this->table == NULL, MQ_OUT_OF_MEMORY);

  LOG_FINEST(( CODELOC, MESSAGECONSUMERTABLE_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
                 "In operationAll(%d) for MessageConsumerTable %p", op, this));

  monitor.enter();
  errorCode = this->table->keyIterationStart();
  if (errorCode == MQ_SUCCESS) {

  while (this->table->keyIterationHasNext()) {
    MQError error = MQ_SUCCESS;
    errorCode = this->table->keyIterationGetNext((const BasicType**)&consumerIDLong);
    if (errorCode == MQ_SUCCESS) {
       errorCode = this->table->getValueFromKey(consumerIDLong, (const Object** const)&consumer);
       if (errorCode == MQ_SUCCESS) {
         LOG_FINE(( CODELOC, MESSAGECONSUMERTABLE_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
             "operationAll(%d) on consumer=0x%p in MessageConsumerTable 0x%p", op, consumer, this ));

         if (op == MessageConsumerTable::CLOSE_CONSUMER) {
           error = consumer->getSession()->closeConsumer(consumer);
           if (error != MQ_SUCCESS) errorCode = error;
           errorCode = this->table->keyIterationStart();
           if (errorCode != MQ_SUCCESS) {
               LOG_SEVERE(( CODELOC, MESSAGECONSUMERTABLE_LOG_MASK, NULL_CONN_ID, errorCode, 
               "operationAll(%d) failed to restart iteration in MessageConsumerTable 0x%p", op, this ));
               break;
           }
         } else if (op == MessageConsumerTable::START_CONSUMER) {
           consumer->start();
         } else if (op == MessageConsumerTable::STOP_CONSUMER) {
           consumer->stop();
           if (error != MQ_SUCCESS) errorCode = error;
         } else if (op == MessageConsumerTable::UNSUBSCRIBE_DURABLE) {
           if (((UTF8String *)opData)->equals(consumer->getSubscriptionName())) {
             errorCode = MQ_CANNOT_UNSUBSCRIBE_ACTIVE_CONSUMER;
             break;
           }
         } else if (op == MessageConsumerTable::RECOVER_RECEIVEQUEUE) {
           errorCode = consumer->getSession()->redeliverMessagesInQueue(
                           consumer->getReceiveQueue(), *((PRBool*)opData));
           if (errorCode != MQ_SUCCESS) break;
         }

       } else {
         LOG_WARNING(( CODELOC, MESSAGECONSUMERTABLE_LOG_MASK, NULL_CONN_ID, errorCode,
            "operationAll(%d): failed to get consumer(consumerID=%s) in MessageConsumerTable 0x%p because '%s' (%d)",
            op, consumerIDLong->toString(), this, errorStr(errorCode), errorCode ));
       }
    } else {
      LOG_WARNING(( CODELOC, MESSAGECONSUMERTABLE_LOG_MASK, NULL_CONN_ID, errorCode,
        "operationAll(%d): failed to get next consumerID in MessageConsumerTable 0x%p because '%s' (%d)",
         op, this, errorStr(errorCode), errorCode ));
    }
  } //while

  } else {
    LOG_WARNING(( CODELOC, MESSAGECONSUMERTABLE_LOG_MASK, NULL_CONN_ID, errorCode,
          "operationAll(%d): failed to start iterating MessageConsumerTable 0x%p because '%s' (%d)",
           op, this, errorStr(errorCode), errorCode ));
  }
  monitor.exit();

  LOG_FINEST(( CODELOC, MESSAGECONSUMERTABLE_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
                 "Exiting operationAll(%d) for MessageConsumerTable %p", op, this));

  return errorCode;
}

