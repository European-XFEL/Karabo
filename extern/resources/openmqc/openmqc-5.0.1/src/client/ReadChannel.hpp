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
 * @(#)ReadChannel.hpp	1.6 06/26/07
 */ 

#ifndef READCHANNEL_HPP
#define READCHANNEL_HPP

#include "../error/ErrorCodes.h"
#include "../basictypes/Runnable.hpp"
#include "../basictypes/Monitor.hpp"
#include "../io/Packet.hpp"

class Connection; // because we can't include Connection.hpp

/**
 * This class is responsible for reading packets from the broker and
 * then dispatching them to the correct ReceiveQueue based on the
 * consumerID in the packet.
 *
 * @see ProtocolHandler::readPacket 
 */
class ReadChannel : public Runnable {
private:
  /** The connection for which this ReadChannel was created. */
  Connection * connection;

  /** True iff a GOODBYE or GOODBYE_REPLY packet has been received.  */
  PRBool receivedGoodBye;

  /** True iff the reader thread was successfully started and running  */
  PRBool isAlive;

  /** True iff an exception occurred in ReadChannel::run */
  PRBool abortConnection;

  /** True if Connection has requested the ReadChannel to exit */
  PRBool closeConnection;

  Monitor monitor;

  MQError initializationError;

  /** The ID of the connection that created this ReadChannel.  It is only used 
   *  so information can be logged after the connection goes away. */
  PRInt64 connectionID;

  /** Initializes all member variables to NULL.  */
  void init();

  /**
   * Using the type of the packet and the consumerID in the packet, this 
   * method dispatches the packet to the appropriate ReceiveQueue.
   * 
   * @param packet the iMQ packet to dispatch
   * @return IMQ_SUCCESS if successful and an error otherwise
   */
  iMQError dispatch(Packet * const packet);

  PRThread * readerThread;
  
public:
  /**
   * Starts the reader thread.
   * @param connection the connection on which this ReadChannel was created.
   * @param Connection:;startThread
   */
  ReadChannel(Connection * const connection);

  /** Destructor.  It assumes that Connection has already called
   *  exitConnection */
  virtual ~ReadChannel();

  /** @return IMQ_SUCCESS if the reader started successfully, and an 
  *   error otherwise */
  iMQError getInitializationError() const;

  /**
   * The entry point for the reader thread.  Until the connection
   * closes, this method reads an iMQ packet from the wire and then
   * calls dispatch to place the packet on the correct ReceiveQueue.
   * If an exception occurs, it calls Connection::exitConnection to
   * close down the connection.
   * 
   * @see dispatch
   * @see Connection::exitConnection
   */
  void run();

  /**
   * Signals the reader thread to exit, and then waits for it to exit.
   * When this method returns, the reader thread has exited.  
   */
  void exitConnection(); 

  PRThread * getReaderThread() const;
  
//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  ReadChannel(const ReadChannel& readChannel);
  ReadChannel& operator=(const ReadChannel& readChannel);
};

#endif
