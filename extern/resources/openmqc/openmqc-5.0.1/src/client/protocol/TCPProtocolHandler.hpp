/*
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS HEADER.
 *
 * Copyright (c) 2010 Oracle and/or its affiliates. All rights reserved.
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
 * @(#)TCPProtocolHandler.hpp	1.4 06/26/07
 */ 

#ifndef TCPPROTOCOLHANDLER_HPP
#define TCPPROTOCOLHANDLER_HPP


#include "../../debug/DebugUtils.h"
#include "../../util/PRTypesUtils.h"
#include "../../containers/Properties.hpp"
#include "../../error/ErrorCodes.h"
#include "../TransportProtocolHandler.hpp"
#include "../../io/TCPSocket.hpp"
#include "../iMQConstants.hpp"
#include <nspr.h>

/**
 * A string defining this transport protocol
 */
static const char * TCP_PROTOCOL_STR = "tcp";

/**
 * TCPProtocolHandler is a TCP-based implementation of the abstract
 * base class TransportProtocolHandler.  It basically acts as an
 * Adapter of the TCPSocket class.  The connect method is the only
 * method that is non-trivial.  
 */
class TCPProtocolHandler : public TransportProtocolHandler {
private:
  /**
   * The actual socket that is used to connect to the broker
   */
  TCPSocket         brokerSocket;

  /**
   * Initializes member variables.  This currently doesn't do anything.
   */
  void init();

  /**
   * Resets brokerSocket.
   */
  void reset();
  
public:
  /**
   * Default constructor.
   */
  TCPProtocolHandler();

  /**
   * Default destructor.  It closes the socket if it hasn't already be closed.
   */
  virtual ~TCPProtocolHandler();
  
  //
  // These are the virtual functions of TransportProtocolHandler that
  // must be implemented.  See the comments in
  // TransportProtocolHandler.hpp to see descriptions of these
  // methods.
  //

  virtual MQError connect(const Properties * const connectionProperties);
  virtual MQError getLocalPort(PRUint16 * const port) const;
  virtual MQError getLocalIP(const IPAddress ** const ipAddr) const;
  virtual MQError read(const PRInt32         numBytesToRead,
                        const PRUint32        timeoutMicroSeconds, 
                              PRUint8 * const bytesRead, 
                              PRInt32 * const numBytesRead);
  virtual MQError write(const PRInt32          numBytesToWrite,
                         const PRUint8 * const  bytesToWrite,
                         const PRUint32         timeoutMicroSeconds, 
                               PRInt32 * const  numBytesWritten);
  virtual MQError close();
  virtual MQError shutdown();
  virtual PRBool isClosed();

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  TCPProtocolHandler(const TCPProtocolHandler& handler);
  TCPProtocolHandler& operator=(const TCPProtocolHandler& handler);

};

#endif // TCPPROTOCOLHANDLER_HPP
