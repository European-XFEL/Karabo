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
 * @(#)JMQBasicAuthenticationHandler.cpp	1.4 06/26/07
 */ 

#include "JMQBasicAuthenticationHandler.hpp"
#include "../../util/UtilityMacros.h"
#include "../../util/LogUtils.hpp"
#include "../../client/iMQConstants.hpp"
#include "../../serial/SerialDataOutputStream.hpp"

#include <base64.h>

/*
 *
 */
JMQBasicAuthenticationHandler::JMQBasicAuthenticationHandler()
{
  CHECK_OBJECT_VALIDITY();

  username       = NULL;
  password       = NULL;
  authProperties = NULL;
}


/*
 *
 */
JMQBasicAuthenticationHandler::~JMQBasicAuthenticationHandler()
{
  CHECK_OBJECT_VALIDITY();

  username       = NULL;
  password       = NULL;
  authProperties = NULL;
}

/*
 *
 */
const char * 
JMQBasicAuthenticationHandler::getType() const
{
  CHECK_OBJECT_VALIDITY();

  return IMQ_AUTHTYPE_JMQBASIC;
}

/*
 *
 */
MQError
JMQBasicAuthenticationHandler::init(const UTF8String * const usernameArg, 
                                    const UTF8String * const passwordArg,
                                    const Properties * const authPropertiesArg)
{
  CHECK_OBJECT_VALIDITY();

  this->username       = usernameArg;
  this->password       = passwordArg;
  this->authProperties = authPropertiesArg;
  return MQ_SUCCESS;
}


/**
 * The method uses NSS utility function BTOA_DataToAscii for base64 encoding.
 * Although NSS header file says this function has been deprecated, this is
 * still the one listed in the latest NSS public function list and the new one
 * that intends to replace this one is not listed.  The base64.h which defines 
 * this function is in SUNWtlsd package, and the Sun Java System webserver still
 * uses this function as well.
 * 
 */
MQError
JMQBasicAuthenticationHandler::handleRequest(const PRUint8 *  const authRequest,
                                             const PRInt32          authRequestLen,
                                                   PRUint8 ** const authReply,
                                                   PRInt32 *  const authReplyLen,
                                             const PRInt32          sequenceNum)
{
  CHECK_OBJECT_VALIDITY();

  ASSERT( authRequestLen == 0 );
  RETURN_ERROR_IF_NULL( authReply );
  RETURN_ERROR_IF_NULL( authReplyLen );
  RETURN_ERROR_IF_NULL( this->username );
  RETURN_ERROR_IF_NULL( this->password );
  // RETURN_ERROR_IF_NULL( this->authProperties ); // not used
  UNUSED( sequenceNum );

  MQError      errorCode           = MQ_SUCCESS;
  char *       pwd64Str            = NULL;
  PRInt32      replyLen            = 0;
  PRUint8 *    reply               = NULL;
  UTF8String * credential          = NULL;
  SerialDataOutputStream out;


  RETURN_ERROR_IF( password->length() == 0, MQ_UNSUPPORTED_ARGUMENT_VALUE );
  pwd64Str = BTOA_DataToAscii((const unsigned char *)password->getCharStr(), password->length());
  if (pwd64Str == NULL) {
    LOG_WARNING(( CODELOC, AUTH_HANDLER_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
               "JMQBasicAuthenticationHandler::handleRequest()"
               " BTOA_DataToAscii failed with NSS error = %d.", PORT_GetError() ));
    return MQ_BASE64_ENCODE_FAILURE;
  }

  MEMCHK( credential = new UTF8String(pwd64Str) );
  PORT_Free(pwd64Str);
  pwd64Str = NULL;

  // Write out the two strings
  ERRCHK( username->write(&out) );
  ERRCHK( credential->write(&out) );
  
  replyLen = out.numBytesWritten();
  ASSERT( replyLen == (username->length() + 2 + credential->length() + 2) );
  reply = new PRUint8[replyLen];
  MEMCHK( reply );
  memcpy( reply, out.getStreamBytes(), replyLen );
    
  *authReply = reply;
  *authReplyLen = replyLen;
  
  DELETE( credential );

  return MQ_SUCCESS;

 Cleanup:
  if (pwd64Str != NULL) PORT_Free(pwd64Str);
  DELETE( credential );
  DELETE( reply );

  return errorCode;
}

/*
 *
 */
MQError
JMQBasicAuthenticationHandler::test()
{
  /* todo */
  return MQ_SUCCESS;
}
