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
 * @(#)TextMessage.hpp	1.3 06/26/07
 */ 

#ifndef TEXTMESSAGE_HPP
#define TEXTMESSAGE_HPP

#include "Message.hpp"

/**
 * This class encapsulates a JMS Text Message.  
 */
class TextMessage : public Message {
private:
  // A copy of the body of the message.
  UTF8String * messageBodyText;

public:
  /**
   * Constructor.
   */
  TextMessage();

  /**
   * Constuctor.
   */
  TextMessage(Packet * const packetArg);

  /**
   * Destructor.
   */
  ~TextMessage();

  /**
   * @return TEXT_MESSAGE
   */
  virtual PRUint16 getType();

  /**
   * Sets the text of the message to messageText.  This method makes a
   * copy of messageText, so the caller is responsible for freeing
   * messageText.
   *
   * @param messageText is the string representation of the message text
   * @return IMQ_SUCCESS if successful and an error otherwise 
   */
  iMQError setMessageText(const UTF8String * const messageText);

  /**
   * Returns the text of the message in messageText.  messageText
   * actually stores a copy of the message text, so the caller is
   * reponsible for freeing messageText.
   *
   * @param messageText is the output parameter for the text of the message
   * @return IMQ_SUCCESS if successful and an error otherwise 
   */
  iMQError getMessageText(UTF8String ** const messageText);


  /** 
   * Similar to setMessageText except a null terminated UTF8-encoded
   * char string is used to initialize the message body.  A copy of
   * the messageText string is made.
   *
   * @param messageText is the string representation of the message text
   * @return IMQ_SUCCESS if successful and an error otherwise 
   * @see setMessageText */
  iMQError setMessageTextString(const char * const messageText);

  /**
   * Similar to getMessageText except a null terminated UTF8-encoded
   * char string is passed back.  The caller should not modify
   * messageText.
   *
   * @param messageText is the output parameter for the text of the message
   * @return IMQ_SUCCESS if successful and an error otherwise 
   * @see getMessageText */
  iMQError getMessageTextString(const char ** const messageText);

  /** @return the type of this object for HandledObject */
  virtual HandledObjectType getObjectType() const;

  /** @return the type of Message */
  virtual HandledObjectType getSuperObjectType() const;

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  TextMessage(const TextMessage& textMessage);
  TextMessage& operator=(const TextMessage& textMessage);

};

#endif // TEXTMESSAGE_HPP
