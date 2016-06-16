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
 * @(#)BytesMessage.hpp	1.3 06/26/07
 */ 

#ifndef BYTESMESSAGE_HPP
#define BYTESMESSAGE_HPP

#include "Message.hpp"

/**
 * This class encapsulates a JMS Bytes Message.  
 */
class BytesMessage : public Message {
private:

public:
  /**
   * Constructor.
   */
  BytesMessage();

  /**
   * Constructor.
   */
  BytesMessage(Packet * const packetArg);

  /**
   * Destructor.
   */
  ~BytesMessage();

  /**
   * @return BYTES_MESSAGE
   */
  virtual PRUint16 getType();

  /**
   * Sets the bytes of the message to messageBytes.  This method makes a
   * copy of messageBytes, so the caller is responsible for freeing
   * messageBytes.
   *
   * @param messageBytes the paylod of the message
   * @param messageBytesSize the size of the paylod of the message
   * @return IMQ_SUCCESS if successful and an error otherwise 
   */
  iMQError setMessageBytes(const PRUint8 * const messageBytes,
                           const PRInt32 messageBytesSize);

  /**
   * Returns the bytes of the message in messageBytes.  A pointer to
   * the actual message bytes are returned.  The caller should not
   * modify or attempt to free messageBytes.
   *
   * @param messageBytes is the output parameter for the bytes of the message
   * @param messageBytesSize is the size of the 
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError getMessageBytes(const PRUint8 ** const messageBytes,
                           PRInt32 * const messageBytesSize) const;

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
  BytesMessage(const BytesMessage& bytesMessage);
  BytesMessage& operator=(const BytesMessage& bytesMessage);
};

#endif // BYTESMESSAGE_HPP
