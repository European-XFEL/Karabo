/*
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS HEADER.
 *
 * Copyright (c) 2000-2013 Oracle and/or its affiliates. All rights reserved.
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
 * @(#)TCPProtocolHandler.cpp	1.9 06/26/07
 */ 

#include "TCPProtocolHandler.hpp"
#include "../../util/UtilityMacros.h"
#include "../../util/LogUtils.hpp"
#include "../PortMapperClient.hpp"

/*
 *
 */
TCPProtocolHandler::TCPProtocolHandler()
{
  CHECK_OBJECT_VALIDITY();

  init();
}



/*
 *
 */
TCPProtocolHandler::~TCPProtocolHandler()
{
  CHECK_OBJECT_VALIDITY();

  reset();
}

/*
 *
 */
void
TCPProtocolHandler::init()
{
  CHECK_OBJECT_VALIDITY();
}

/*
 *
 */
void
TCPProtocolHandler::reset()
{
  CHECK_OBJECT_VALIDITY();

  brokerSocket.reset();
}


/*
 *
 */
MQError 
TCPProtocolHandler::connect(const Properties * const connectionProperties)
{
  CHECK_OBJECT_VALIDITY();

  iMQError errorCode = MQ_SUCCESS;

  PortMapperClient portMapperClient;
  UTF8String tcpProtocol(TCP_PROTOCOL_STR);
  UTF8String normalConnection(CONNECTION_TYPE_NORMAL_STR);
  PRInt32 directPort = 0;
  PRUint16 brokerPort = 0;
  const char * brokerName = NULL;
  PRBool useIPV6 = PR_FALSE;
  PRUint32 connectTimeout = 0;

  // Make sure we are not already connected, and that
  // connectionProperties is valid
  CNDCHK( brokerSocket.status() != TCPSocket::NOT_CONNECTED, 
          MQ_TCP_ALREADY_CONNECTED );
  NULLCHK( connectionProperties );

  // Get the host name property first 
  ERRCHK( connectionProperties->getStringProperty(MQ_BROKER_HOST_PROPERTY,
                                                  &brokerName) );
  NULLCHK(brokerName);
  if ( connectionProperties->getIntegerProperty(MQ_SERVICE_PORT_PROPERTY,
                                               &directPort) == MQ_SUCCESS) {
    CNDCHK( (directPort <= 0 || 
             directPort > PORT_MAPPER_CLIENT_MAX_PORT_NUMBER), MQ_TCP_INVALID_PORT );
    brokerPort = directPort;
  }
  errorCode = connectionProperties->getBooleanProperty(
                MQ_ENABLE_IPV6_PROPERTY, &useIPV6); 
  if (errorCode != MQ_SUCCESS && errorCode != MQ_NOT_FOUND) {
      ERRCHK( errorCode);
  }
  if (errorCode == MQ_NOT_FOUND) {
    useIPV6 = PR_FALSE;
  }

  if (brokerPort == 0) {
    // Use the portmapper to find out what port to connect to
    ERRCHK( portMapperClient.readBrokerPorts(connectionProperties) );
    ERRCHK( portMapperClient.getPortForProtocol(&tcpProtocol,
                                                &normalConnection,
                                                &brokerPort) );
  }
  
  // We should probably get this from a property
  connectTimeout = DEFAULT_CONNECT_TIMEOUT;

  // Now connect to the broker on the JMS port
  ERRCHK( this->brokerSocket.connect(brokerName, 
                                     brokerPort, useIPV6,
                                     connectTimeout) );

  LOG_INFO(( CODELOC, TCP_HANDLER_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
             "Opened TCP connection to broker %s:%d.", brokerName, brokerPort ));
  
  return MQ_SUCCESS;

Cleanup:
  LOG_SEVERE(( CODELOC, TCP_HANDLER_LOG_MASK, NULL_CONN_ID, 
               MQ_COULD_NOT_CONNECT_TO_BROKER,
               "Could not open TCP connection to broker %s:%d because '%s' (%d)", 
               (brokerName == NULL ? "NULL":brokerName), 
               brokerPort, errorStr(errorCode), errorCode ));
  
  MQ_ERROR_TRACE("connect", errorCode);
  return errorCode;
}


/*
 *
 */
MQError 
TCPProtocolHandler::getLocalPort(PRUint16 * const port) const
{
  CHECK_OBJECT_VALIDITY();

  return brokerSocket.getLocalPort(port);
}

/*
 *
 */
MQError 
TCPProtocolHandler::getLocalIP(const IPAddress ** const ipAddr) const
{
  CHECK_OBJECT_VALIDITY();

  return brokerSocket.getLocalIP(ipAddr);
}


/*
 *
 */
MQError 
TCPProtocolHandler::read(const PRInt32          numBytesToRead,
                         const PRUint32         timeoutMicroSeconds, 
                               PRUint8 * const  bytesRead, 
                               PRInt32 * const  numBytesRead)
{
  CHECK_OBJECT_VALIDITY();

  return brokerSocket.read(numBytesToRead,
                           timeoutMicroSeconds,
                           bytesRead,
                           numBytesRead);
}


/*
 *
 */
MQError 
TCPProtocolHandler::write(const PRInt32          numBytesToWrite,
                          const PRUint8 * const  bytesToWrite,
                          const PRUint32         timeoutMicroSeconds, 
                                PRInt32 * const  numBytesWritten)
{
  CHECK_OBJECT_VALIDITY();

  return brokerSocket.write(numBytesToWrite,
                            bytesToWrite,
                            timeoutMicroSeconds,
                            numBytesWritten);
}

/*
 *
 */
MQError 
TCPProtocolHandler::close()
{
  CHECK_OBJECT_VALIDITY();

  return brokerSocket.close();
}


/*
 *
 */
MQError 
TCPProtocolHandler::shutdown()
{
  CHECK_OBJECT_VALIDITY();

  return brokerSocket.shutdown();
}

/*
 *
 */
PRBool
TCPProtocolHandler::isClosed()
{
  CHECK_OBJECT_VALIDITY();

  return brokerSocket.isClosed();
}

