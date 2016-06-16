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
 * @(#)BytesMessage.cpp	1.3 06/26/07
 */ 

#include "BytesMessage.hpp"
#include "../util/UtilityMacros.h"
#include "../io/PacketType.hpp"

/*
 *
 */
BytesMessage::BytesMessage() : Message()
{
  CHECK_OBJECT_VALIDITY();

  if (packet != NULL) {
    packet->setPacketType(PACKET_TYPE_BYTES_MESSAGE);
  }
}

/*
 *
 */
BytesMessage::BytesMessage(Packet * const packetArg) : Message(packetArg)
{
  CHECK_OBJECT_VALIDITY();

  ASSERT( packetArg != NULL );
  ASSERT( packetArg->getPacketType() == PACKET_TYPE_BYTES_MESSAGE );
}

/*
 *
 */
BytesMessage::~BytesMessage()
{
  CHECK_OBJECT_VALIDITY();
}


/*
 *
 */
PRUint16 
BytesMessage::getType()
{
  CHECK_OBJECT_VALIDITY();

  return PACKET_TYPE_BYTES_MESSAGE;
}

/*
 *
 */
iMQError
BytesMessage::setMessageBytes(const PRUint8 * const messageBytes,
                              const PRInt32 messageBytesSize)
{
  iMQError errorCode = IMQ_SUCCESS;
  PRUint8 * messageBytesCopy = NULL;
  
  NULLCHK( messageBytes );
  CNDCHK( this->packet == NULL, IMQ_OUT_OF_MEMORY );
  
  // Copy the bytes
  MEMCHK( messageBytesCopy = new PRUint8[messageBytesSize] );
  memcpy( messageBytesCopy, messageBytes, messageBytesSize );
  
  // Set the message body.
  this->packet->setMessageBody(messageBytesCopy, messageBytesSize);
  messageBytesCopy = NULL;
  
  return IMQ_SUCCESS;
Cleanup:
  DELETE( messageBytesCopy );
  
  return errorCode;
}

/*
 *
 */
iMQError
BytesMessage::getMessageBytes(const PRUint8 ** const messageBytes,
                              PRInt32 * const messageBytesSize) const
{
  iMQError errorCode = IMQ_SUCCESS;

  NULLCHK( messageBytes );
  NULLCHK( messageBytesSize );
  CNDCHK( this->packet == NULL, IMQ_OUT_OF_MEMORY );

  *messageBytes = this->packet->getMessageBody();
  *messageBytesSize = this->packet->getMessageBodySize();
  
  return IMQ_SUCCESS;
Cleanup:
  return errorCode;
}

/*
 *
 */
HandledObjectType
BytesMessage::getObjectType() const 
{
  CHECK_OBJECT_VALIDITY();

  return BYTES_MESSAGE_OBJECT;
}

/*
 *
 */
HandledObjectType
BytesMessage::getSuperObjectType() const
{
  CHECK_OBJECT_VALIDITY();

  return MESSAGE_OBJECT;
}
