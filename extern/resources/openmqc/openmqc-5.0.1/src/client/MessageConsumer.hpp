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
 * @(#)MessageConsumer.hpp	1.20 11/09/07
 */ 

#ifndef MESSAGECONUMSER_HPP
#define MESSAGECONUMSER_HPP

#include <nspr.h>
#include "../error/ErrorCodes.h"
#include "Destination.hpp"
#include "ReceiveQueue.hpp"
#include "MessageConsumer.hpp"
#include "Message.hpp"
#include "ReceiveMode.h"
#include "../basictypes/HandledObject.hpp"
#include "../cshim/mqcallback-types-priv.h"

class Session;

/**
 * Implements topic subscribers and queue receivers.
 */
class MessageConsumer : public HandledObject {
protected:
  /** The Session that created this MessageConsumer */
  Session * session;

  /** The destination for packets consumed by this MessageConsumer */
  Destination * destination;

  /** The incoming queue for messages destined for this MessageConsumer */
  ReceiveQueue * receiveQueue;

  /** The ID unique to this connection that identifies this consumer */
  PRInt64 consumerID; 

  /** True iff destination is a queue */
  PRBool isTopic;

  /** True iff this MessageConsumer is durable */
  PRBool isDurable;

  /** True iff this MessageConsumer is shared */
  PRBool isShared;

  /** True iff this MessageConsumer should not consume messages produced
   *  by this connection */
  PRBool noLocal;

  /** The subscription name.  NULL if not durable and not shared */
  const UTF8String * subscriptionName;

  const UTF8String * messageSelector;

  /** The optional callback function that notifies a consumer that a
   *  message has arrived */
  MQMessageArrivedFunc messageArrivedCallback;

  /** The void* user data that was passed to setMessageArrivedCallback.
   *  It is passed back to messageArrivedCallback */
  void * messageArrivedCallbackData;

  MQMessageListenerFunc    messageListener;
  void *    messageListenerCallbackData;

  ReceiveMode receiveMode;
  PRInt32 prefetchMaxMsgCount;
  PRFloat64 prefetchThresholdPercent;

private:


  /** True iff the message consumer has been properly initialized and registered */
  PRBool isInitialized;

  // true if consumerID has been assigned   
  PRBool registered;

  /** If initialization fails in the constructor, the errorCode that led to the
   *  failure is stored here. */
  MQError initializationError;

  PRBool isDMQConsumer;

  /** initializes member variables to NULL/false */
  void init();

  PRBool isClosed;

  SysMessageID lastDeliveredSysMessageID; 
  PRBool hasLastDeliveredSysMessageID; 

  MQError beforeMessageListener(const Message * message, MQError mqerror);
  void afterMessageListener(const Message * message, MQError mqerror);

public:
  /**
   * Constructor.
   * 
   * @param session the Session that created this MessageConsumer
   * @param destination the Destination that this MessageConsumer receives messages on
   * @param isDurable true iff this is a durable MessageConsumer
   * @param durableName if isDurable, then this is the name of the durable consumer
   * @param messageSelector 
   * @param noLocal true iff the consumer should not receive messages produced
   *        by this connection
   * @param messageListener
   * @param messageListenerCallbackData
   * @param destination 
   */
  MessageConsumer(Session * const session, 
                  Destination * const destination,
                  const PRBool isDurable,
                  const PRBool isShared,
                  const UTF8String * const subscriptionName, 
                  const UTF8String * const messageSelector, 
                  const PRBool noLocal,
                  MQMessageListenerFunc messageListener,
                  void *                messageListenerCallbackData);

  ~MessageConsumer();

  /** @return the type of this object for HandledObject */
  virtual HandledObjectType getObjectType() const;
  
  // Accessors
  const Destination * getDestination() const;
  PRInt64 getConsumerID() const;
  PRBool isRegistered() const;
  void setConsumerID(PRInt64 id);
  PRInt32 getAckMode() const;
  PRInt32 getPrefetchMaxMsgCount() const;
  PRFloat64 getPrefetchThresholdPercent() const;
  void setPrefetchMaxMsgCount(PRInt32 prefetchSize);
  void setPrefetchThresholdPercent(PRFloat64 prefetchThreshold);
  ReceiveMode getReceiveMode() const;
  PRBool getIsTopic() const; 
  PRBool getIsDurable() const;
  PRBool getIsShared() const;
  PRBool getNoLocal() const;
  const UTF8String * getSubscriptionName() const;
  const UTF8String * getMessageSelector() const;
  ReceiveQueue * getReceiveQueue() const;
  PRBool getIsInitialized() const;
  virtual MQError getInitializationError() const;
  Session * getSession() const; 
  PRBool getHasLastDeliveredSysMessageID() const;
  const SysMessageID * getLastDeliveredSysMessageID() const;

  /** Called by ReceiveQueue::enqueueNotify to notify the consumer that a 
      message has been enqueued */
  void messageEnqueued() const;

  /** Set the callback for when a message arrives for this consumer */
  MQError setMessageArrivedCallback(const MQMessageArrivedFunc messageArrivedFunc,
                                     void * messageArrivedFuncData);

  /**
   * If there is a message waiting to be received, then it is returned
   * in message.  Otherwise, this method immediately returns.
   *
   * @param message the output parameter for the received message
   * @return MQ_SUCCESS if successful and an error otherwise 
   */
  MQError receiveNoWait(Message ** const message);
  
  /**
   * Block until a message for this consumer arrives or an exception
   * occurs on the connection.
   *
   * @param message the output parameter for the received message
   * @return MQ_SUCCESS if successful and an error otherwise 
   */
  MQError receive(Message ** const message);


  /**
   * Block until a message for this consumer arrives,
   * timeoutMilliSeconds elapse, or an exception occurs on the
   * connection.
   *
   * @param message the output parameter for the received message
   * @param timeoutMilliSeconds the number of milliseconds to wait for 
   *        a message to arrive
   * @return MQ_SUCCESS if successful and an error otherwise */
  MQError receive(Message ** const message, 
                   const PRUint32 timeoutMilliSeconds);

  MQError onMessage(Message * message, PRBool * messageListenerInvoked);


  /**
   * Closes this consumer.
   * @return MQ_SUCCESS if successful and an error otherwise 
   */
  MQError close();

  /**
   * for sync message consumer
   */
  void stop();

  /**
   * for sync message consumer
   */
  void start();

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  MessageConsumer(const MessageConsumer& messageConsumer);
  MessageConsumer& operator=(const MessageConsumer& messageConsumer);
};

#endif // MESSAGECONUMSER_HPP

