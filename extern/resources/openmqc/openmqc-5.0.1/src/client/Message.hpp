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
 * @(#)Message.hpp	1.9 06/26/07
 */ 

#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include "../io/Packet.hpp"
#include "Destination.hpp"
#include "../basictypes/HandledObject.hpp"
#include <nspr.h>


class Session;

/** 
 * This is the lowest overhead delivery mode because it does not
 * require that the message be logged to stable storage. The level of
 * JMS provider failure that causes a NON_PERSISTENT_DELIVERY message
 * to be lost is not defined.
 *
 * A JMS provider must deliver a NON_PERSISTENT_DELIVERY message with
 * an at-most-once guarantee. This means it may lose the message but
 * it must not deliver it twice.  
 */
static const PRInt32 NON_PERSISTENT_DELIVERY = 1;

/**
 * This mode instructs the JMS provider to log the message to stable
 * storage as part of the client's send operation. Only a hard media
 * failure should cause a PERSISTENT_DELIVERY message to be lost.
 */
static const PRInt32 PERSISTENT_DELIVERY = 2;

/**
 * This class very closely follows the Java iMQ MessageImpl class.  It
 * encapsulates a JMS message.  It is an abstract base class for
 * actual message types such as TextMessage.  
 */
class Message : public HandledObject {
protected:
  /**
   * The Message class acts as an adaptor for most of the fields of
   * packet.  
   */
  Packet * packet;

  /**
   * Destination (if any) for this message
   */
  const Destination * dest;


  /**
   * Reply to destination (if any) for this message
   */
  Destination * replyToDest; 

  /**
   * cache for received message */
  SysMessageID sysMessageID;
  PRUint64     consumerID;


  /**
   * The session (if any) that is associated with this message. */
  const Session * session;

  PRBool ackProcessed;

private:
  /**
   * Initializes member variables.
   */
  void init();
  
  /**
   * Deallocates all memory associated with this packet.
   */
  void reset();

public:
  /**
   * Constructor.  It allocates a new Packet to base this message on. */
  Message();

  /**
   * Constructor.  It bases the message on the packet parameter.
   * 
   * @param packet the Packet to base the Message on.
   */
  Message(Packet * const packet);

  /**
   * Destructor.
   */
  virtual ~Message();

  /** @return IMQ_SUCCESS if the constructor was successful and an
      error otherwise.  The default constructor could fail to allocate
      a packet if we've run out of memory. */
  virtual iMQError getInitializationError() const;

  /**
   * @return the type of the message as defined in PacketType.
   */
  virtual PRUint16 getType();

  /**
   * @return the iMQ packet corresponding to this JMS message 
   */
  Packet * getPacket();


  /**
   * Constructs a new Message of a specific type based on the type of
   * the packet. 
   * 
   * @param packet the packet to use to construct the message
   * @return a Message of the type specified in packet (e.g. TEXT_MESSAGE) */
  static Message* createMessage(Packet * const packet);

  //
  // These are accessors for the packet fields.  These can be made
  // virtual as needed.  
  //
  
  iMQError setJMSMessageID(UTF8String * const messageID);
  iMQError getJMSMessageID(const UTF8String ** const messageID);

  iMQError setJMSTimestamp(const PRInt64 timestamp);
  iMQError getJMSTimestamp(PRInt64 * const timestamp);

  iMQError setJMSCorrelationID(UTF8String * const correlationID);
  iMQError getJMSCorrelationID(const UTF8String ** const correlationID);

  iMQError setJMSReplyTo(const Destination * const replyTo);
  iMQError getJMSReplyTo(const Destination ** const replyTo);

  iMQError setJMSDestination(const Destination * const destination);
  iMQError getJMSDestination(const Destination ** const destination);

  iMQError setJMSDeliveryMode(const PRInt32 deliveryMode);
  iMQError getJMSDeliveryMode(PRInt32 * const deliveryMode);

  iMQError setJMSRedelivered(const PRBool redelivered);
  iMQError getJMSRedelivered(PRBool * const redelivered);
  
  iMQError setJMSType(UTF8String * const messageType);
  iMQError getJMSType(const UTF8String ** const messageType);
  
  iMQError setJMSExpiration(const PRInt64 expiration);
  iMQError getJMSExpiration(PRInt64 * const expiration);

  iMQError setJMSDeliveryTime(const PRInt64 deliveryTime);
  iMQError getJMSDeliveryTime(PRInt64 * const deliveryTime);

  iMQError setJMSPriority(const PRUint8 priority);
  iMQError getJMSPriority(PRUint8 * const priority);

  iMQError setProperties(Properties * const properties);
  iMQError getProperties(const Properties ** const properties);

  iMQError setHeaders(Properties * const headers);
  iMQError getHeaders(Properties ** const headers) const;

  PRUint64 getConsumerID() const;
  const SysMessageID * getSystemMessageID() const; 
  
  void setSession(const Session * session);
  const Session * getSession() const;
  PRBool isAckProcessed() const;
  void setAckProcessed();
  PRBool isExpired() const;

  /** @return the type of this object for HandledObject */
  virtual HandledObjectType getObjectType() const;

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  Message(const Message& message);
  Message& operator=(const Message& message);
};


#endif // MESSAGE_HPP
