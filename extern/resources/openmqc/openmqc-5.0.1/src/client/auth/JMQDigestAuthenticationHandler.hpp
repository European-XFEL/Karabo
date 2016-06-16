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
 * @(#)JMQDigestAuthenticationHandler.hpp	1.3 06/26/07
 */ 

#ifndef JMQDIGESTAUTHENTICATIONHANDLER_HPP
#define JMQDIGESTAUTHENTICATIONHANDLER_HPP

#include "../../auth/AuthenticationProtocolHandler.hpp"

/**
 * The length of an MD5 hash in bytes.
 */
static const PRInt32 MD5_HASH_LEN = 16; // 128 bits

/**
 * This class implements the client side of the iMQ Digest authentication
 * method.
 */
class JMQDigestAuthenticationHandler : public AuthenticationProtocolHandler {
private:
  /**
   * The username to use for the authentication.
   */
  const UTF8String * username;

  /**
   * The password to use for the authentication.
   */
  const UTF8String * password; 

  /**
   * The authentication properties to use for the authentication.  This is 
   * currently not used.
   */
  const Properties * authProperties;

  /**
   * Calculates the MD5 hash of buf and converts the result to a
   * character string of hex digits.
   *
   * @param buf the buffer to hash
   * @param bufLen the length of buf
   * @param hashedBufStr the output parameter where the output string is placed.
   *  The caller is responsible for freeing this string.
   *
   * @return IMQ_SUCCESS if succesful and an error otherwise.  
   */
  static iMQError getMD5HashString(const char *  const buf,
                                   const PRInt32       bufLen,
                                         char ** const hashedBufStr);

  /**
   * This method converts a 128-bit MD5 hash value (interpreted as a 128-bit
   * 2's complement number) into the absolute value of the number and the
   * sign of the number.
   *
   * @param hashedBuf the buffer to convert.
   * @param signedHashBuf output parameter for the absolute value of the number.
   * @param isNegative output parameter that stores if the original number was
   *  negative.
   */
  static void convertMD5HashToSigned(const PRUint8 hashedBuf[MD5_HASH_LEN], 
                                           PRUint8 signedHashBuf[MD5_HASH_LEN], 
                                           PRBool * const isNegative);
  /**
   * Tests convertMD5HashToSigned. 
   *
   * @return IMQ_SUCCESS if succesful and an error otherwise.  
   * @see convertMD5HashToSigned
   */
  static iMQError testConvertMD5HashToSigned();


public:
  /**
   * Constructor.
   */
  JMQDigestAuthenticationHandler();

  /**
   * Destructor.
   */
  virtual ~JMQDigestAuthenticationHandler();

  virtual iMQError init(const UTF8String * const username, 
                        const UTF8String * const password,
                        const Properties * const authProperties);

  virtual iMQError handleRequest(const PRUint8 *  const authRequest,
                                 const PRInt32          authRequestLen,
                                       PRUint8 ** const authReply,
                                       PRInt32 *  const authReplyLen,
                                 const PRInt32          sequenceNum);

  virtual const char * getType() const;

  /**
   * The static test method that tests the general functionality of this class.
   */
  static iMQError test();

};


#endif // JMQDIGESTAUTHENTICATIONHANDLER_HPP

