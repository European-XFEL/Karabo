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
 * @(#)AuthenticationProtocolHandler.hpp	1.4 06/28/07
 */ 

#ifndef AUTHENTICATIONPROTOCOLHANDLER_HPP
#define AUTHENTICATIONPROTOCOLHANDLER_HPP

#include <nspr.h>
#include "../error/ErrorCodes.h"
#include "../basictypes/UTF8String.hpp"
#include "../containers/Properties.hpp"
#include "../basictypes/Object.hpp"


/**
 * AuthenticationProtocolHandler provides the client-side API for an
 * application implementor to plug-in their own authentication request
 * handler.  This allows the client to authenticate with the broker.
 */
class AuthenticationProtocolHandler : public Object {
public:
  virtual ~AuthenticationProtocolHandler() {}

  
  /**
   * This method is called right before start a authentication process
   *
   * @param username the user name passed from createConnection()
   * @param password the password passed from createConnection()
   * @param authProperties not defined yet 
   *
   * Currently for JMQ2.0, username/password always have values (if not
   * passed in createConnection() call, they are assigned default values).
   *
   */
  virtual iMQError init(const UTF8String * const username, 
                        const UTF8String * const password,
                        const Properties * const authProperties) = 0;


  /**
   * This method is called to handle a authentication request.
   *
   * @param authRequest the authentication request data.  This is then
   *        packet body of AUTHENTICATE_REQUEST packet.
   * @param authRequestLen is the length of authRequest
   * @param authReply is the output request length.  This will be the
   *        packet body of AUTHENTICATE packet.  handleRequest allocates
   *        this buffer, and the caller is responsible for freeing it.
   * @param authReplyLen is the length of authReply
   * @param sequenceNum this is the sequence number field in the 
   *        AUTHENTICATE_REQUEST packet.  It can be used for correlation 
   *        in multiple requests case.
   * @return IMQ_SUCCESS if successful and an error otherwise.
   * 
   */
  virtual iMQError handleRequest(const PRUint8 *  const authRequest,
                                 const PRInt32          authRequestLen,
                                       PRUint8 ** const authReply,
                                       PRInt32 *  const authReplyLen,
                                 const PRInt32          sequenceNum) = 0;


  /**
   * @return the type of authentication
   */
  virtual const char * getType() const = 0;
};


#endif // AUTHENTICATIONPROTOCOLHANDLER_HPP

