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
 * @(#)Session.hpp	1.21 10/17/07
 */ 

#ifndef SESSION_HPP
#define SESSION_HPP

#include "../util/UtilityMacros.h"
#include "../error/ErrorCodes.h"
#include "../basictypes/HandledObject.hpp"
#include "AckMode.h"
#include "ReceiveMode.h"
#include "Destination.hpp"
#include "MessageConsumer.hpp"
#include "MessageProducer.hpp"
#include "MessageConsumerTable.hpp"
#include "SessionMutex.hpp"

// We can't include Connection.hpp due to circular reference so we
// include it in the cpp file.  
class Connection;
class SessionQueueReader;

/**
 * The Session class encapsulates an iMQ Session.  This includes
 * facilities to create destinations, message producers, and message
 * senders. */
class Session : public HandledObject {
protected:
  /** True iff the session has been properly initialized */
  MQError initializationError;

  /** Acknowledgement mode.  Currently, must be CLIENT_ACKNOWLEDGE. */
  AckMode        ackMode;

  PRInt64        transactionID;
  PRBool         isXA;

  /** True iff the session is closed. */
  PRBool isClosed;

  /** Pointer back to the connection that created this Session. */
  Connection * connection;

  ReceiveMode receiveMode;

private:
  PRInt64        sessionID;

  ReceiveQueue       *  sessionQueue;
  SessionQueueReader *  sessionQueueReader;
  PRThread           *  sessionThread;

  /** A list of producers that were created by this session. */
  Vector producers;
  Monitor producersMonitor;

  MessageConsumerTable  consumerTable;

  PRBool isStopped;
  
  /** True iff the session is transacted.  Currently, must be FALSE. */
  PRBool isTransacted;

private:

  PRInt32        dupsOkLimit;

  /** A monitor that serializes closing the session */
  Monitor closeMonitor;

  SessionMutex   sessionMutex;

  /** Initializes all member variables to false/NULL. */
  void init();

  /** This holds messages that have been delivered but not
   *  acknowledged so we can acknowledge a series of messages.  This
   *  vector holds objects of type MessageID. */
  ObjectVector unAckedMessageQueue;
  SerialDataOutputStream ackBlockStream;
  SerialDataOutputStream ackExpiredBlockStream;

  /** 
   *  @return IMQ_SUCCESS if the session is not closed and an error
   *  otherwise */
  iMQError checkSessionState();

  MQError acknowledgeMessage(const Message * const messageID);
  MQError redeliverUnAckedMessages();


public:

  /**
   * Constructor.  Only stores arguments and initializes member
   * variables.  Currently, isTransacted must be false and ackMode
   * must be CLIENT_ACKNOWLEDGE.
   *
   * @param connection the connection that created this Session
   * @param isTransacted controls if this ocnnection is transacted
   * @param ackMode the mode of acknowledgement for this connection
   *  (e.g. AUTO_ACKNOWLEDGE, CLIENT_ACKNOWLEDGE, DUPS_OK_ACKNOWLEDGE) 
   * @param receiveMode */
  Session(      Connection * const connection,
          const PRBool             isTransacted, 
          const AckMode            ackMode,
          const ReceiveMode        receiveMode);

  /**
   * Destructor.  Calls close, which closes all producers and
   * consumers that were created by this session.  */
  virtual ~Session();
  
  PRBool getIsXA() const;
  ReceiveMode getReceiveMode() const;
  AckMode getAckMode() const;
  PRInt64 getSessionID() const;
  void setSessionID(PRInt64 sessionIDArg);
  PRInt64 getTransactionID() const;

  ReceiveQueue * getSessionQueue() const;

  /**
   * Closes the session and all producers and consumers that were
   * created by this session.
   *
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError close(PRBool clean);
  iMQError start();
  iMQError stop();

  /**
   * Calls Connection::registerMessageProducer to register a producer
   * for the specified destination.
   *
   * @param destination the Destination to register a producer for
   * @return IMQ_SUCCESS if successful and an error otherwise 
   * @see Connection::registerMessageProducer */
  MQError registerMessageProducer(const Destination * const destination, PRInt64 * producerID);
  MQError unregisterMessageProducer(PRUint64 producerID);

  /**
   * Closes producer.  This includes deleting producer.
   * @param producer the producer to close
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError closeProducer(MessageProducer * producer);

  /**
   * Closes consumer.  This includes deleting consumer.
   * @param consumer the consumer to close
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError closeConsumer(MessageConsumer * consumer);

  /**
   * Creates a Destination with the provided name.
   *
   * @param name the name of the Destination to create
   * @param isQueue true if name specifies a queue, and false if it's a topic
   * @param destination the output parameter for the created Destination
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError createDestination(const UTF8String * const name, 
                             const PRBool isQueue,
                             Destination ** const destination);

  /**
   * Creates a temporary Destination that is guaranteed to be unique.
   *
   * @param isQueue true if name specifies a queue, and false if it's a topic
   * @param destination the output parameter for the created Destination
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError createTemporaryDestination(const PRBool isQueue,
                                      Destination ** const destination);

  MQError redeliverMessagesInQueue(ReceiveQueue *queue, PRBool setRedelivered);
private:

public:

  /** @return the Connection that created this Session  */
  Connection * getConnection();


  /** @return true iff this Session is closed */
  PRBool getIsClosed() const;

  PRBool getIsStopped() const;

  /**
   * Creates a messageProducer that can send to multiple destinations.
   *
   * @param messageProducer the output parameter for the MessageProducer that 
   *        is created
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError createProducer(MessageProducer ** const messageProducer);

  /**
   * Creates a messageProducer to topic.
   *
   * @param destination the Destination to send to
   * @param producer the output parameter for the MessageProducer that 
   *        is created
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError createProducer(Destination * const destination, 
                          MessageProducer ** const producer);


  /**
   * Creates a MessageConsumer for the specified Destination
   *
   * @param destination the Destinatino to consume from
   * @param isDurable true iff this is a durable consumer
   * @param isShared true iff this is a shared consumer
   * @param subscriptionName if isDurable or isShared, then this is subscription name
   * @param messageSelector
   * @param noLocal true iff the consumer should not receive messages produced
   *        by this connection
   * @param messageListener
   * @param messageListenerCallbackData
   * @param consumer the output parameter for the created MessageConsumer
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError createConsumer(Destination * const destination,
                          const PRBool isDurable,
                          const PRBool isShared,
                          const UTF8String * const subscriptionName,
                          const UTF8String * const messageSelector,
                          const PRBool noLocal,
                          MQMessageListenerFunc messageListener,
                          void *                messageListenerCallbackData,
                          MessageConsumer ** const consumer);


  MQError commit();
  MQError rollback();
  MQError recover(PRBool fromRollback);
  MQError startTransaction();

  /**
   * Sends message to the destination specified in the message.
   *
   * @param message the Message to send 
   * @return IMQ_SUCCESS if successful and an error otherwise */
  virtual MQError writeJMSMessage(Message * const message, PRInt64 producerID);

  /** @return the type of this object for HandledObject */
  virtual HandledObjectType getObjectType() const;

  /**
   * Message acknowledgement processing or pre-processing
   *
   * @param message the message that has been delivered to the consumer
   * @param useAckMode 
   * @return MQ_SUCCESS if successful and an error otherwise */
  MQError acknowledge(Message * message, AckMode useAckMode, PRBool fromMessageListener);

  virtual MQError acknowledge(Message * message, PRBool fromMessageListener);

  MQError acknowledgeExpiredMessage(const Message * const messageID);

  /**
   * acknowledge messages */
  MQError acknowledgeMessages(PRBool fromClientAcknowledge, Message * message);

  /**
   * a message has been delivered from the session - used by flowcontrol */
  void messageDelivered();

  MQError addConsumer(PRUint64 consumerID, MessageConsumer * const consumer);
  MQError getConsumer(PRUint64 consumerID, MessageConsumer ** const consumer);
  MQError removeConsumer(PRUint64 consumerID, MessageConsumer ** const Consumer);
  
  virtual MQError getInitializationError() const;

  /**
   * Unit test method. 
   *
   * @param session the session to test
   * @return IMQ_SUCCESS if successful and an error otherwise
   */
  static iMQError test(Session * const session);

  /**
   * Unit test to test durable consumer. 
   *
   * @param connect the Connection to create sessions with
   * @return IMQ_SUCCESS if successful and an error otherwise
   */
  static iMQError testDurableConsumer(Connection * const connection);

  /**
   * Calls Connection::unsubscribeDurableConsumer to unsubscribe from
   * the broker the durable consumer specified by durableName.  This
   * only applies to topic subscribers.
   *
   * @param durableName the name of the durable consumer to unsubscribe
   * @return IMQ_SUCCESS if successful and an error otherwise  
   * @see Connection::unsubscribeDurableConsumer*/
  iMQError unsubscribeDurableConsumer(const UTF8String * const durableName);

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  Session(const Session& session);
  Session& operator=(const Session& session);
};


#endif

