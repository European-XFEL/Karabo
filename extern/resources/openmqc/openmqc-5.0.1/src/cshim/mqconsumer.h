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
 * @(#)mqconsumer.h	1.12 06/26/07
 */ 

#ifndef MQ_CONSUMER_H
#define MQ_CONSUMER_H

/*
 * declarations of C interface for message consumer
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mqtypes.h"
  
/**
 * Closes the message consumer.  
 *
 * @param consumerHandle the handle to the consumer to close
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQCloseMessageConsumer(MQConsumerHandle consumerHandle);

/**
 * Waits until the consumer specified by consumerHandle receives a
 * message and returns this message in messageHandle.  If there is
 * already a message pending for this consumer, then this call returns
 * it immediately and does not block.  If an exception occurs, such as
 * the connection closing before a message arrives, then this call
 * returns with an error.
 *
 * @param consumerHandle the handle to the consumer to wait for a message
 *        to arrive
 * @param messageHandle the output parameter that contains the received
 *        message
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQReceiveMessageWait(const MQConsumerHandle consumerHandle, 
                     MQMessageHandle *      messageHandle);

/**
 * Waits for up to timeoutMilliSeconds milliseconds until the consumer
 * specified by consumerHandle receives a message and returns this
 * message in messageHandle.  If there is already a message pending
 * for this consumer, then this call returns it immediately and does
 * not block.  If an exception occurs before a message arrives or the
 * timeout expires, such as the connection closing, then this call
 * returns with an error.
 *
 * @param consumerHandle the handle to the consumer to wait for a message
 *        to arrive
 * @param timeoutMilliSeconds the number of milliseconds to wait for a
 *        message to arrive for this consumer         
 * @param messageHandle the output parameter that contains the received
 *        message
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQReceiveMessageWithTimeout(const MQConsumerHandle consumerHandle, 
                            MQInt32                timeoutMilliSeconds,
                            MQMessageHandle *      messageHandle);

/**
 * If a message is pending for the consumer, then this call
 * immediately returns it.  Otherwise, it immediately returns an
 * error.
 *
 * @param consumerHandle the handle to the consumer to wait for a message
 *        to arrive
 * @param messageHandle the output parameter that contains the received
 *        message
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQReceiveMessageNoWait(const MQConsumerHandle consumerHandle, 
                       MQMessageHandle *      messageHandle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MQ_CONSUMER_H */
