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
 * @(#)TCPSocket.hpp	1.7 06/26/07
 */ 

#ifndef TCPSOCKET_HPP
#define TCPSOCKET_HPP

#include "../debug/DebugUtils.h"
#include "../util/PRTypesUtils.h"
#include "../containers/Properties.hpp"
#include "../error/ErrorCodes.h"
#include "../client/TransportProtocolHandler.hpp"
#include "../basictypes/Object.hpp"
#include "../basictypes/Monitor.hpp"
#include <nspr.h>
#include <stdio.h>

/**
 * This class implements a TCP connection to a host. 
 */
class TCPSocket : public Object {
public:
  /** The Status type encodes whether the connection is connected or not. */
  enum Status {CONNECTED, NOT_CONNECTED};
private:
  /**
   * The socket file descriptor that is used to access the connection
   * to the broker.  
   */
public:
  PRFileDesc * hostSocket;

private:

  /**
   * The address entry for the host that is returned by PR_GetHostByName.  It
   * contains pointers into the hostEntryData field.  
   */
  PRHostEnt    hostEntry;

  /**
   * hostEntryData contains data returned by PR_GetHostByName.
   * hostEntry has pointers that point into this field.
   */
  char         hostEntryData[PR_NETDB_BUF_SIZE];

  /**
   * The address entry for the localhost that is returned by PR_GetHostByName.  It
   * contains pointers into the localEntryData field.  
   */
  PRHostEnt    localEntry;

  /**
   * localEntryData contains data returned by PR_GetHostByName.
   * localEntry has pointers that point into this field.
   */
  char         localEntryData[PR_NETDB_BUF_SIZE];

  /** the address (including port number) of the host */
  PRNetAddr    hostAddr;
  
  /** the address (including port number) of the local connection to the host */
  PRNetAddr    localAddr;

  /** 
   * localPort is the host order port number of the local connection to the host 
   */
  PRUint16     localPort;


  /**
   * local IP address
   */
  IPAddress    localIP;


  /* These are only used for debugging */
  FILE * writtenBytesFile;
  FILE * readBytesFile;
  
  /**
   * This method initializes all members of the class.
   */
  void init();
  iMQError cacheLocalAddr();
  iMQError pollForEvents(PRPollDesc * const pd, const PRIntervalTime timeout);
  iMQError setAcceptSocket(PRFileDesc * const acceptingSocket);

public:
  /**
   * Constructor.
   */
  TCPSocket();

  /**
   * Used for debugging purposes.
   */
  TCPSocket(const char * const debugFileBase);
  
  /**
   * Destructor.
   */
  virtual ~TCPSocket();

  

  /**
   * This method closes the connection (if open) and deallocates all memory
   * associated with this connection.  
   */
  virtual void reset();

  /**
   * This method connects to the host specified by hostName and hostPort.
   *
   * @param hostName the name of the host to conenct to
   * @param hostPort the port of the host to connect to
   * @param timeoutMicroSeconds the number of microseconds to wait to connect.  
   *        A value of 0 implies do not wait, and a value of 0xFFFFFFFF implies 
   *        wait forever.
   * @return IMQ_SUCCESS if successful and an error otherwise.
   */
  virtual iMQError connect(const char * hostName, 
                           const PRUint16 hostPort, const PRBool useIPV6,
                           const PRUint32 timeoutMicroSeconds);

  /**
   * This method is called immediately before the call to PR_Connect.  It
   * is primarily used by SSLSocket to allow it to setup the SSL properties.
   *
   * @return IMQ_SUCCESS if successful and an error otherwise.
   */
  virtual iMQError preConnect(const char * hostName);

  /**
   * This method does the connect
   *
   * @param addr A pointer to the address of the peer to which this socket
   *        is to be connected
   * @param timeout The time limit for completion of the connect operation.
   */
  virtual MQError doConnect(PRNetAddr *addr, PRIntervalTime timeout, const char * hostName);

  /**
   * This method is called immediately after the connection to the
   * host completes.  It is used primarily by SSLSocket to handle some
   * SSL specific activities.
   *
   * @return IMQ_SUCCESS if successful and an error otherwise.
   */
  virtual iMQError postConnect();
  
  virtual iMQError setDefaultSockOpts();
  
  /**
   * This method reads numBytesToRead bytes from the connection and places the
   * results in bytesRead.
   *
   * @param numBytesToRead is the number of bytes to read from the connection
   * @param timeoutMicroSeconds the number of microseconds to wait for the read 
   *        to complete.  A value of 0 implies do not wait, and a value of 
   *        0xFFFFFFFF implies wait forever.
   * @param bytesRead is the buffer where the bytes read from the input stream 
   *        are placed.
   * @param numBytesRead is the number of bytes that were actually read from 
   *        the connection.
   * @return IMQ_SUCCESS if successful and an error otherwise.  
   */
  virtual iMQError read(const PRInt32          numBytesToRead,
                        const PRUint32         timeoutMicroSeconds, 
                              PRUint8 * const  bytesRead, 
                              PRInt32 * const  numBytesRead);

  /**
   * This method writes numBytesToWrite bytes from the buffer bytesToWrite to
   * the connection.
   *
   * @param numBytesToWrite is the number of bytes to write to the connection
   * @param bytesToWrite is the buffer where the bytes are written from
   * @param timeoutMicroSeconds the number of microseconds to wait for the write
   *        to complete.  A value of 0 implies do not wait, and a value of 
   *        0xFFFFFFFF implies wait forever.
   * @param numBytesWritten is the number of bytes that were actually written to
   *        the connection.
   * @return IMQ_SUCCESS if successful and an error otherwise.  
   */
  virtual iMQError write(const PRInt32          numBytesToWrite,
                         const PRUint8 * const  bytesToWrite,
                         const PRUint32         timeoutMicroSeconds, 
                               PRInt32 * const  numBytesWritten);

  /**
   * This method shuts down the connection to the host, but does not
   * close the socket or file descriptor.
   *
   * @return IMQ_SUCCESS if successful and an error otherwise.  
   */
  virtual iMQError shutdown();

  /**
   * This method closes the connection to the host.
   *
   * @return IMQ_SUCCESS if successful and an error otherwise.  
   */
  virtual iMQError close();

  /**
   * This method returns the local port number of the connection to the host.
   *
   * @param port is where the local port number is placed
   * @return IMQ_SUCCESS if successful and an error otherwise.  
   */
  virtual iMQError getLocalPort(PRUint16 * const port) const;

  /**
   * This method returns the local ip address of the connection to the host.
   *
   * @param ipAddr is where the local ipAddr is placed
   * @return IMQ_SUCCESS if successful and an error otherwise.  
   */
  virtual iMQError getLocalIP(const IPAddress ** const ipAddr) const;

  /**
   * This method returns the status of the connection (connected or not
   * connected).  
   *
   * @return TCPSocket::CONNECTED if the socket is connected and 
   *          TCPSocket::NOT_CONNECTED if the socket is not connected.
   */
  virtual Status status() const;

  /**
   * This method returns whether the connection is closed.
   *
   * @return PR_TRUE iff the connection is closed
   */
  virtual PRBool isClosed();


  /**
   * Binds -- used only for testing.
   */
  virtual iMQError bind(const char * localName, 
                        const PRUint16 localPort);

  /**
   * Listens -- used only for testing.
   */
  virtual iMQError listen();

  /**
   * Accepts -- used only for testing.
   */
  virtual iMQError accept(TCPSocket ** const acceptingTCPSocket,
                          const PRUint32 timeoutMicroSeconds);

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  TCPSocket(const TCPSocket& tcpSocket);
  TCPSocket& operator=(const TCPSocket& tcpSocket);
};


#endif // TCPSOCKET_HPP
