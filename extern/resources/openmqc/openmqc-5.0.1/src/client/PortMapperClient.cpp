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
 * @(#)PortMapperClient.cpp	1.7 06/26/07
 */ 

#include "PortMapperClient.hpp"
#include "../util/UtilityMacros.h"
#include "TransportProtocolHandler.hpp"
#include "../io/TCPSocket.hpp"


PortMapperClient::PortMapperClient()
{
  CHECK_OBJECT_VALIDITY();

}

MQError
PortMapperClient::readBrokerPorts(const Properties * const connectionProperties)
{
  CHECK_OBJECT_VALIDITY();
  MQError errorCode = MQ_SUCCESS;

  portMapperTable.reset();

  // Get the host name property
  const char * brokerName = NULL;
  errorCode = connectionProperties->getStringProperty(
                MQ_BROKER_HOST_PROPERTY, &brokerName);
  if (errorCode != MQ_SUCCESS) {
    MQ_ERROR_TRACE( "readBrokerPort", errorCode );
    return errorCode;
  }

  // Get the host port property
  PRInt32 brokerPort = 0;
  errorCode = connectionProperties->getIntegerProperty(
                MQ_BROKER_PORT_PROPERTY, &brokerPort);
  if (errorCode != MQ_SUCCESS) {
    MQ_ERROR_TRACE( "readBrokerPort", errorCode );
    return errorCode;
  }
  if ((brokerPort < 0) || (brokerPort > PORT_MAPPER_CLIENT_MAX_PORT_NUMBER)) {
    return MQ_TCP_INVALID_PORT;
  }

  PRBool useIPV6 = PR_FALSE;
  errorCode = connectionProperties->getBooleanProperty(
                MQ_ENABLE_IPV6_PROPERTY, &useIPV6);
  if (errorCode != MQ_SUCCESS && errorCode != MQ_NOT_FOUND) {
    MQ_ERROR_TRACE( "readBrokerPort", errorCode );
    return errorCode;
  }
  if (errorCode == MQ_NOT_FOUND) {
    useIPV6 = PR_FALSE;
  }

  // Open up a new connection to the broker
  TCPSocket brokerSocket;
  RETURN_IF_ERROR_TRACE( brokerSocket.connect(brokerName,
                                        (PRUint16)brokerPort, useIPV6,
                                        PORT_MAPPER_CLIENT_CONNECT_MICROSEC_TIMEOUT), "readBrokerPorts", "mq");

  PRInt32 numBytesWritten = 0;
  PRInt32 numBytesToWrite = STRLEN(PORTMAPPER_VERSION_LINE);
  errorCode = brokerSocket.write(numBytesToWrite,
                                 (const PRUint8 *)PORTMAPPER_VERSION_LINE,
                                 PR_INTERVAL_NO_TIMEOUT,
                                 &numBytesWritten);
  /* This can sometimes fail if the server already wrote
   * the port table and closed the connection */
  if (errorCode != MQ_SUCCESS) { 
    LOG_FINE(( CODELOC, SOCKET_LOG_MASK, NULL_CONN_ID,
               MQ_PORTMAPPER_ERROR,
               "Failed to write port mapper version to broker because '%s' (%d)",
               errorStr(errorCode), errorCode ));
    MQ_ERROR_TRACE( "readBrokerPort", errorCode );
  } else if (numBytesWritten != numBytesToWrite) {
    LOG_SEVERE(( CODELOC, SOCKET_LOG_MASK, NULL_CONN_ID,
                 MQ_PORTMAPPER_ERROR,
                 "Unexpected bytes %d written to broker for port mapper version but expected %d",
                 numBytesWritten, numBytesToWrite ));
    MQ_ERROR_TRACE( "readBrokerPort", MQ_PORTMAPPER_ERROR );
    return  MQ_PORTMAPPER_ERROR;
  }

  // Read the port server output into a buffer
  PRUint8 portMappings[PORT_MAPPER_CLIENT_MAX_PORT_MAPPINGS_SIZE];
  PRInt32 numBytesRead = 0;

  errorCode = brokerSocket.read(sizeof(portMappings),
                                PORT_MAPPER_CLIENT_RECEIVE_MICROSEC_TIMEOUT,
                                portMappings,
                                &numBytesRead);

  // The broker sends the port mappings and then closes the socket, so
  // we may get an error back even though we successfully read the port
  // mappings
  if ((errorCode != MQ_SUCCESS) && (numBytesRead <= 0)) {
    MQ_ERROR_TRACE( "readBrokerPort", errorCode );
    return errorCode;
  }

  // Put the buffer in a string
  UTF8String portMapStr((char*)portMappings, numBytesRead);

  // Parse the string and put the results in a lookup table
  RETURN_IF_ERROR_TRACE( portMapperTable.parse(&portMapStr), "readBrokerPorts", "mq" );

  return MQ_SUCCESS;
}



MQError
PortMapperClient::getPortForProtocol(const UTF8String * const protocol, 
                                     const UTF8String * const type,
                                           PRUint16   * const port) 

{
  CHECK_OBJECT_VALIDITY();

  MQError errorCode = MQ_SUCCESS;

  // Get the port entry
  const PortMapperEntry * portMapperEntry = NULL;
  errorCode = portMapperTable.getPortForProtocol(
                protocol, type, &portMapperEntry);
  if (errorCode != MQ_SUCCESS) {
    LOG_SEVERE(( CODELOC, SOCKET_LOG_MASK, NULL_CONN_ID, errorCode,
                 "Failed to get port for protocol %s", protocol->getCharStr() ));
    return errorCode;
  }

  ASSERT( portMapperEntry != NULL );
  *port = portMapperEntry->getPort();
  
  return MQ_SUCCESS;
}


