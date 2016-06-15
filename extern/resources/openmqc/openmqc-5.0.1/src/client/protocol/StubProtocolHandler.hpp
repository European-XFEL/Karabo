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
 * @(#)StubProtocolHandler.hpp	1.3 06/26/07
 */ 

#ifndef STUBPROTOCOLHANDLER_HPP
#define STUBPROTOCOLHANDLER_HPP

#include "../../debug/DebugUtils.h"
#include "../../containers/Properties.hpp"
#include "../../error/ErrorCodes.h"
#include "../TransportProtocolHandler.hpp"
#include "../../serial/SerialDataInputStream.hpp"
#include "../../serial/SerialDataOutputStream.hpp"

#include <nspr.h>


/**
 * This class is only used for testing/debugging purposes.
 */
static const int STUB_PROTOCOL_HANDLER_MAX_BUFFER_SIZE = 100 * 1000;
class StubProtocolHandler : public TransportProtocolHandler {
private:
  SerialDataInputStream inputStream;
  SerialDataOutputStream outputStream;

  PRUint8 buffer[STUB_PROTOCOL_HANDLER_MAX_BUFFER_SIZE];
  PRUint32 bufferSize;

  const char * fileToWrite; 
  
public:

  StubProtocolHandler(const char * fileToRead, const char * fileToWrite);
  ~StubProtocolHandler();

  // Methods from TransportProtocolHandler that must be filled in.
  virtual iMQError connect(const Properties * const connectionProperties);
  virtual iMQError getLocalPort(PRUint16 * const port) const;
  virtual iMQError getLocalIP(const IPAddress ** const ipAddr) const;
  virtual iMQError read(const PRInt32          numBytesToRead,
                        const PRUint32         timeoutMicroSeconds, 
                              PRUint8 * const  bytesRead, 
                              PRInt32 * const  numBytesRead);
  virtual iMQError write(const PRInt32          numBytesToWrite,
                         const PRUint8 * const  bytesToWrite,
                         const PRUint32         timeoutMicroSeconds, 
                               PRInt32 * const  numBytesWritten);
  virtual iMQError shutdown();
  virtual iMQError close();

  
  virtual PRBool isClosed();

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  StubProtocolHandler(const StubProtocolHandler& stubProtocolHandler);
  StubProtocolHandler& operator=(const StubProtocolHandler& stubProtocolHandler);
};


#endif // STUBPROTOCOLHANDLER_HPP
