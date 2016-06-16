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
 * @(#)ReadQTable.hpp	1.6 06/26/07
 */ 

#ifndef READQTABLE_HPP
#define READQTABLE_HPP

#include "../containers/BasicTypeHashtable.hpp"
#include "../basictypes/Monitor.hpp"
#include "../basictypes/Object.hpp"
#include "ReceiveQueue.hpp"
#include <nspr.h>

class Packet;

/** 
 * This class maps a consumerID to a ReceiveQueue.  A consumerID is a
 * unique number that is associated with a subscriber, receiver, or
 * ack receiver (i.e. ProtocolHandler).  It allows packets from the
 * broker to be demultiplexed to the appropriate place.  A
 * ReceiveQueue is where incoming packets are placed.  */
class ReadQTable : public Object {
private:
  /**
   * A hashtable that maps a consumerID to its ReceiveQueue
   */
  BasicTypeHashtable *  table;

  /**
   * Ensures synchronous access to the hashtable.
   */
  Monitor monitor;

  /** The next id  -  currently only used when for ack qtable */  
  PRInt64                          nextID;

  void getNextID(PRInt64 * const id);
  
public:
  /**
   * Initializes the hashtable so that the hashtable will automatically
   * delete each consumerID and ReceiveQueue when it is destructed.
   */
  ReadQTable();

  /**
   * Deconstructor that deletes the hashtable.
   */
  virtual ~ReadQTable();
  
  /**
   * Removes the ReceiveQueue associated with consumerID from the
   * ReadQTable.  The ReceiveQueue associated with consumerID is also
   * deleted.
   *  
   * @param consumerID is the consumerID to remove from the table
   * @return MQ_SUCCESS if successful and an error otherwise 
   */
  MQError remove(const PRInt64 consumerID);

  /**
   * Adds the consumerID to receiveQ mapping to the ReadQTable.
   *
   * @param consumerID is the consumerID to add to the table
   * @param receiveQ is the ReceiveQueue to associate with consumerID
   * @return MQ_SUCCESS if successful and an error otherwise 
   */
  MQError add(const PRInt64 consumerID, ReceiveQueue * const receiveQ);

  /**
   * Generate a consumerID and add the receiveQ to the ReadQTable.
   *
   * @param consumerID output parameter for consumerID 
   * @param receiveQ is the ReceiveQueue to associate with consumerID
   * @return MQ_SUCCESS if successful and an error otherwise 
   */
  MQError add(PRInt64 * consumerID, ReceiveQueue * const receiveQ);


  /**
   * @param consumerID is the consumerID to add to the table
   * @param receiveQ is the output parameter for the ReceiveQueue associated 
   *        with consumerID
   * @return MQ_SUCCESS if successful and an error otherwise 
   */
  MQError get(const PRInt64 consumerID, ReceiveQueue ** const receiveQ);

  MQError enqueue(const PRInt64 consumerID, Packet * const packet);

  MQError closeAll();


  /**
   * Static method that tests the general functionality of this class.
   */
  static MQError test();

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  ReadQTable(const ReadQTable& readQTable);
  ReadQTable& operator=(const ReadQTable& readQTable);
};

#endif // READQTABLE_HPP

