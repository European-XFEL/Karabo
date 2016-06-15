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
 * @(#)SSLSocket.hpp	1.6 06/26/07
 */ 

#ifndef SSLSOCKET_HPP
#define SSLSOCKET_HPP

#include "TCPSocket.hpp"
#include <nss.h>


/**
 * This class implements a TCP connection to a host. 
 */
class SSLSocket : public TCPSocket {
private:
  PRBool          hostIsTrusted;
  PRBool          useCertMD5Hash;
  UTF8String *    hostCertificateMD5HashStr;

  // true iff SSL has been initialized
  static PRBool   initialized;
  
  void init();
  void ntCancelIO();

public:
  /**
   * Constructor.
   */
  SSLSocket();

  /**
   *
   */
  virtual ~SSLSocket();

  /**
   *
   */
  virtual void reset();

  /**
   *
   */
  virtual iMQError setSSLParameters(const PRBool hostIsTrusted,
                                    const PRBool useCertMD5Hash,
                                    const char * const hostCertMD5HashStr);
  
  /**
   * This method is called immediately before the call to PR_Connect.  It
   * sets some SSL specific options.
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
   * host completes.  It resets the SSL handshake so we don't run into
   * problems using blocking sockets.
   *
   * @return IMQ_SUCCESS if successful and an error otherwise.  */
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
   *
   */
  virtual SECStatus checkBadCertificate(PRFileDesc * const socket);


  /**
   *
   */
  static iMQError initializeSSL(const char * const certificateDirectory);

  
//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  SSLSocket(const SSLSocket& sslSocket);
  SSLSocket& operator=(const SSLSocket& sslSocket);
};


#endif // SSLSOCKET_HPP












