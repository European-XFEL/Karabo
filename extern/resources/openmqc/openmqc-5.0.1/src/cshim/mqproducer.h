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
 * @(#)mqproducer.h	1.15 06/26/07
 */ 
 
#ifndef MQ_PRODUCER_H
#define MQ_PRODUCER_H

/*
 * declarations of C interface for message producer
 */ 

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mqtypes.h"
#include "mqmessage.h"
  
/**
 * Closes the message producer.  
 *
 * @param producerHandle the handle to the producer to close
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQCloseMessageProducer(MQProducerHandle producerHandle);

/**
 * Has the producer specified by producerHandle send the message
 * specified by messageHandle to the producer's destination with
 * the default message properties.  This call can only be used with 
 * a producer that has a specified destination at creation time (i.e.
 * producers created by calling MQCreateMessageProducerForDestination)
 *
 * @param producerHandle the handle to the producer to close
 * @param messageHandle the message to send
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQSendMessage(const MQProducerHandle producerHandle,
              const MQMessageHandle  messageHandle);

/**
 * Has the producer specified by producerHandle send the message
 * specified by messageHandle to the producer's destination with the
 * specified message properties.  This call can only be used with a
 * producer that has a specified destination at creation time (i.e.
 * producers created by calling MQCreateMessageProducerForDestination)
 *
 * @param producerHandle the handle to the producer to close
 * @param messageHandle the message to send
 * @param msgDeliveryMode the persistent delivery mode of the
 *        message.  Options are MQ_NON_PERSISTENT_DELIVERY and
 *        MQ_PERSISTENT_DELIVERY
 * @param msgPriority the priority of the message. There are 10 levels
 *        of priority, with 0 lowest and 9 highest. The default level
 *        is 4. A JMS provider tries to deliver higher-priority
 *        messages before lower-priority ones, but does not have to
 *        deliver messages in exact order of priority.
 * @param msgTimeToLive the message's lifetime (in milliseconds)
 *        If the specified value is zero, the message never expires.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQSendMessageExt(const MQProducerHandle producerHandle,
                 const MQMessageHandle  messageHandle,
                 MQDeliveryMode         msgDeliveryMode,
                 MQInt8                 msgPriority,
                 MQInt64                msgTimeToLive);

/**
 * Has the producer specified by producerHandle send the message
 * specified by messageHandle to the destination specified by
 * destinationHandle with the default message properties. This 
 * call can only be used with a producer that does not have a
 * specified destination at creation time (i.e. producers created
 * by calling MQCreateMessageProducer)
 *
 * @param producerHandle the handle to the producer to close
 * @param messageHandle the message to send
 * @param destinationHandle the destination to send the message to
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQSendMessageToDestination(const MQProducerHandle    producerHandle,
                           const MQMessageHandle     messageHandle,
                           const MQDestinationHandle destinationHandle);

/**
 * Has the producer specified by producerHandle send the message
 * specified by messageHandle to the destination specified by
 * destinationHandle with the specified message properties.  This 
 * call can only be used with a producer that does not have a specified
 * destination at creation time (i.e. producers created by calling
 * MQCreateMessageProducer)
 *
 * @param producerHandle the handle to the producer to close
 * @param messageHandle the message to send
 * @param destinationHandle the destination to send the message to
 * @param msgDeliveryMode the persistent delivery mode of the
 *        message.  Options are MQ_NON_PERSISTENT_DELIVERY and
 *        MQ_PERSISTENT_DELIVERY
 * @param msgPriority the priority of the message. There are 10 levels
 *        of priority, with 0 lowest and 9 highest. The default level
 *        is 4. A JMS provider tries to deliver higher-priority
 *        messages before lower-priority ones, but does not have to
 *        deliver messages in exact order of priority.
 * @param msgTimeToLive the number of milliseconds until the
 *        message expires.  If the specified value is zero, the message
 *        never expires.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQSendMessageToDestinationExt(const MQProducerHandle    producerHandle,
                              const MQMessageHandle     messageHandle,
                              const MQDestinationHandle destinationHandle,
                              MQDeliveryMode            msgDeliveryMode,
                              MQInt8                    msgPriority,
                              MQInt64                   msgTimeToLive);

/**
 * Set the minimum length of time in milliseconds from its dispatch time
 * before a produced message becomes visible on the target destination and 
 * available for delivery to consumers.
 */
EXPORTED_SYMBOL MQStatus
MQSetDeliveryDelay(const MQProducerHandle producerHandle,
                   MQInt64 deliveryDelay);

/**
 * Get the minimum length of time in milliseconds from its dispatch time
 * before a produced message becomes visible on the target destination and 
 * available for delivery to consumers.
 */
EXPORTED_SYMBOL MQStatus
MQGetDeliveryDelay(const MQProducerHandle producerHandle,
                   MQInt64 * deliveryDelay);
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MQ_PRODUCER_H */

