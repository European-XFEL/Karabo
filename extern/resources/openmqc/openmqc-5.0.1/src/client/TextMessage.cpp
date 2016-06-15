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
 * @(#)TextMessage.cpp	1.3 06/26/07
 */ 

#include "TextMessage.hpp"
#include "../util/UtilityMacros.h"
#include "../io/PacketType.hpp"

/*
 *
 */
TextMessage::TextMessage() : Message()
{
  CHECK_OBJECT_VALIDITY();

  if (packet != NULL) {
    packet->setPacketType(PACKET_TYPE_TEXT_MESSAGE);
  }

  messageBodyText = NULL;
}

/*
 *
 */
TextMessage::TextMessage(Packet * const packetArg) : Message(packetArg)
{
  CHECK_OBJECT_VALIDITY();

  ASSERT( packetArg != NULL );
  ASSERT( packetArg->getPacketType() == PACKET_TYPE_TEXT_MESSAGE );

  messageBodyText = NULL;
}

/*
 *
 */
TextMessage::~TextMessage()
{
  CHECK_OBJECT_VALIDITY();

  DELETE( messageBodyText );
}


/*
 *
 */
PRUint16 
TextMessage::getType()
{
  CHECK_OBJECT_VALIDITY();

  return PACKET_TYPE_TEXT_MESSAGE;
}


/*
 *
 */
iMQError
TextMessage::setMessageText(const UTF8String * const messageText)
{
  CHECK_OBJECT_VALIDITY();

  iMQError errorCode = IMQ_SUCCESS;
  const PRUint8 * textBytes = NULL;
  PRInt32 textBytesSize = 0;
  PRUint8 * textBytesCopy = NULL;
  SerialDataOutputStream textStream;

  NULLCHK( messageText );

  CNDCHK( packet == NULL, IMQ_OUT_OF_MEMORY );

  // Get the raw UTF8
  textBytes = messageText->getBytes();
  textBytesSize = messageText->getBytesSize();
  ASSERT( (textBytes != NULL) || (textBytesSize == 0) );
  ASSERT( textBytesSize >= 0 );

  // Copy the bytes
  MEMCHK( textBytesCopy = new PRUint8[textBytesSize] );
  memcpy( textBytesCopy, textBytes, textBytesSize );
  textBytes = NULL;

  // Set the message body.
  packet->setMessageBody(textBytesCopy, textBytesSize);
  textBytesCopy = NULL;

  return IMQ_SUCCESS;
Cleanup:

  DELETE_ARR( textBytesCopy );
  return errorCode;
}


/*
 *
 */
iMQError
TextMessage::getMessageText(UTF8String ** const messageText)
{
  CHECK_OBJECT_VALIDITY();

  iMQError errorCode = IMQ_SUCCESS;
  NULLCHK( messageText );
  *messageText = NULL;

  CNDCHK( packet == NULL, IMQ_OUT_OF_MEMORY );
  MEMCHK( *messageText = new UTF8String((const char*)packet->getMessageBody(), 
                                        packet->getMessageBodySize()) );
  
  return IMQ_SUCCESS;
Cleanup:
  DELETE( *messageText );
  return errorCode;
}

/*
 *
 */
iMQError 
TextMessage::getMessageTextString(const char ** const messageText)
{
  CHECK_OBJECT_VALIDITY();

  iMQError errorCode = IMQ_SUCCESS;

  NULLCHK( messageText );
  *messageText = NULL;
  CNDCHK( packet == NULL, IMQ_OUT_OF_MEMORY );
  
  // Delete the current copy of the message body
  DELETE( messageBodyText );

  // Allocate a new string for the message body
  MEMCHK( messageBodyText = new UTF8String((const char*)packet->getMessageBody(), 
                                           packet->getMessageBodySize()) );
  
  // Return the output string
  *messageText = messageBodyText->getCharStr();

  return IMQ_SUCCESS;
Cleanup:
  return errorCode;
}

/*
 *
 */
iMQError 
TextMessage::setMessageTextString(const char * const messageText)
{
  CHECK_OBJECT_VALIDITY();

  iMQError errorCode = IMQ_SUCCESS;
  const PRUint8 * textBytes = NULL;
  PRInt32 textBytesSize = 0;
  PRUint8 * textBytesCopy = NULL;
  SerialDataOutputStream textStream;

  NULLCHK( messageText );
  CNDCHK( packet == NULL, IMQ_OUT_OF_MEMORY );

  // Get the raw UTF8
  textBytes = (const PRUint8*) messageText;
  textBytesSize = (PRInt32)STRLEN( messageText );
  ASSERT( (textBytes != NULL) || (textBytesSize == 0) );
  ASSERT( textBytesSize >= 0 );

  // Copy the bytes
  MEMCHK( textBytesCopy = new PRUint8[textBytesSize] );
  memcpy( textBytesCopy, textBytes, textBytesSize );
  textBytes = NULL;

  // Set the message body.
  packet->setMessageBody(textBytesCopy, textBytesSize);
  textBytesCopy = NULL;

  return IMQ_SUCCESS;
Cleanup:

  DELETE_ARR( textBytesCopy );
  return errorCode;
}


/*
 *
 */
HandledObjectType
TextMessage::getObjectType() const 
{
  CHECK_OBJECT_VALIDITY();

  return TEXT_MESSAGE_OBJECT;
}



/*
 *
 */
HandledObjectType
TextMessage::getSuperObjectType() const
{
  CHECK_OBJECT_VALIDITY();

  return MESSAGE_OBJECT;
}

