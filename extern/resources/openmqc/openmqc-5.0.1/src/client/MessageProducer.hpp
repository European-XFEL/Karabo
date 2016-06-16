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
 * @(#)MessageProducer.hpp	1.5 06/26/07
 */ 

#ifndef MESSAGEPRODUCER_HPP
#define MESSAGEPRODUCER_HPP

#include "Destination.hpp"
#include "Message.hpp"
#include "../basictypes/HandledObject.hpp"

class Session; // can't have mutual inclusion

/** These are default values for message header values that are set by
 *  a producer. */
static const PRInt32 MESSAGE_PRODUCER_DEFAULT_DELIVERY_MODE = PERSISTENT_DELIVERY;
static const PRInt32 MESSAGE_PRODUCER_DEFAULT_PRIORITY = 4;
static const PRInt32 MESSAGE_PRODUCER_MIN_PRIORITY = 0;
static const PRInt32 MESSAGE_PRODUCER_MAX_PRIORITY = 9;
static const PRInt64 MESSAGE_PRODUCER_DEFAULT_TIME_TO_LIVE = LL_INIT( 0, 0 );  // 0
static const PRInt64 MESSAGE_PRODUCER_DEFAULT_DELIVERY_DELAY = LL_INIT( 0, 0 );  // 0

/**
 * This class implements a topic publisher and a queue sender.  
 */
class MessageProducer : public HandledObject {
private:
  MQError initializationError;
protected:
  PRBool isClosed;
  /**
   * Session that created this MessageProducer.
   */
  Session * session;

  /**
   * Destination where messages are sent.
   */
  Destination * destination;

  /**
   * Destinations that this producer can send to. 
   */
  BasicTypeHashtable validatedDestinations;
  Monitor monitor;

  /**
   * Whether message delivery is persistent or non-persistent.  Soec
   * defaults it to persistent.  
   */
  PRInt32 deliveryMode;

  /**
   * The JMS API defines ten levels of priority value, with 0 as the
   * lowest priority and 9 as the highest. Clients should consider
   * priorities 0-4 as gradations of normal priority and priorities
   * 5-9 as gradations of expedited priority. Priority is set to 4 by
   * default.
   */
  PRInt32 priority;

  /**
   * The default length of time in milliseconds from its dispatch time
   * that a produced message should be retained by the message system.
   * Time to live is set to zero by default.
   */
  PRInt64 timeToLive;

  PRInt64 deliveryDelay;

  /**
   * Sends message to the with the default destination, delivery mode,
   * priority, and time-to-live.
   *
   * @param message the Message to send 
   * @return IMQ_SUCCESS if successful and an error otherwise
   */
  iMQError writeJMSMessage(Message * const message);

  /**
   * Sends message to the with the default destination, delivery mode,
   * priority, and time-to-live.
   *
   * @param message the Message to send 
   * @param msgDestination the Destination to send the message to
   * @return IMQ_SUCCESS if successful and an error otherwise
   */
  iMQError writeJMSMessage(Message * const message,
                           const Destination * const msgDestination);

  /**
   * Sends message to the specified destination, with the specified
   * delivery mode, priority, and time-to-live.
   *
   * @param message the Message to send 
   * @param msgDeliveryMode persistent or non-persistent delivery
   * @param msgPriority the delivery priority of the message
   * @param msgTimeToLive the time-to-live of the message
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError writeJMSMessage(Message * const message,
                           const Destination * const msgDestination,
                           const PRInt32 msgDeliveryMode,
                           const PRInt8  msgPriority,
                           const PRInt64 msgTimeToLive);

  /**
   * Sends message to the default destination, with the specified
   * delivery mode, priority, and time-to-live.
   *
   * @param message the Message to send 
   * @param msgDeliveryMode persistent or non-persistent delivery
   * @param msgPriority the delivery priority of the message
   * @param msgTimeToLive the time-to-live of the message
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError writeJMSMessage(Message * const message,
                           const PRInt32 msgDeliveryMode,
                           const PRInt8  msgPriority,
                           const PRInt64 msgTimeToLive);

private:
  /**
   * Initializes all member variables to default values.
   */
  void init();

public:
  /**
   * Constructor.
   *
   * @param session the Session that created this MessageProducer
   */
  MessageProducer(Session * const session);

  /**
   * Constructor.
   *
   * @param session the Session that created this MessageProducer
   * @param destination the Destination where this MessageProducer sends messages
   */
  MessageProducer(Session * const session, Destination * const destination);

  virtual MQError getInitializationError() const;

  /**
   * Destructor.
   */
  virtual ~MessageProducer();

  /**
   * @return the Destination where this producer sends messages
   */
  const Destination * getDestination() const;


  /**
   * Sends message to the destination.
   *
   * @param message the Message to send
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError send(Message * const message);  

  /**
   * Sends message to the destination.
   *
   * @param message the Message to send
   * @param msgDestination the Destination to send the message to
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError send(Message * const message, const Destination * const msgDestination);

  /**
   * Sends message to the default destination, with the specified
   * delivery mode, priority, and time-to-live.
   *
   * @param message the Message to send 
   * @param msgDeliveryMode persistent or non-persistent delivery
   * @param msgPriority the delivery priority of the message
   * @param msgTimeToLive the time-to-live of the message
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError send(Message * const message,
                const PRInt32 msgDeliveryMode,
                const PRInt8  msgPriority,
                const PRInt64 msgTimeToLive);

  /**
   * Sends message to the specified Destination, with the specified
   * delivery mode, priority, and time-to-live.
   *
   * @param message the Message to send 
   * @param msgDestination the Destination to send the message to
   * @param msgDeliveryMode persistent or non-persistent delivery
   * @param msgPriority the delivery priority of the message
   * @param msgTimeToLive the time-to-live of the message
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError send(Message * const message,
                const Destination * const msgDestination,
                const PRInt32 msgDeliveryMode,
                const PRInt8  msgPriority,
                const PRInt64 msgTimeToLive);


  /**
   * Validates with the broker that this producer can send to
   * msgDestination.
   *
   * @param msgDestination the Destination to validate
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError validateDestination(const Destination * const msgDestination, PRInt64 * producerID);

  
  /** Accessors */
  PRInt32 getDeliveryMode() const;
  PRInt32 getPriority() const;
  PRInt64 getTimeToLive() const;
  PRInt64 getDeliveryDelay() const;
  void setDeliveryMode(const PRInt32 deliveryMode);
  void setPriority(const PRInt32 priority);
  void setTimeToLive(const PRInt64 timeToLive);
  void setDeliveryDelay(const PRInt64 deliveryDelay);

  /** @return the session that created this Producer */
  Session * getSession() const; 

  /**
   * Closes the MessageProducer.
   *
   * @return IMQ_SUCCESS if successful and an error otherwise
   */
  iMQError close();

  /** Needed to implement HandledObject */
  virtual HandledObjectType getObjectType() const;

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  MessageProducer(const MessageProducer& messageProducer);
  MessageProducer& operator=(const MessageProducer& messageProducer);
};


#endif // MESSAGEPRODUCER_HPP
