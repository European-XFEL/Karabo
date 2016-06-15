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
 * @(#)SysMessageID.hpp	1.3 06/26/07
 */ 

#ifndef SYSMESSAGEID_HPP
#define SYSMESSAGEID_HPP

#include <prtypes.h>
#include "../net/IPAddress.hpp"
#include "../error/ErrorCodes.h"
#include "../io/IMQDataInputStream.hpp"
#include "../io/IMQDataOutputStream.hpp"
#include "../basictypes/UTF8String.hpp"
#include "../basictypes/Object.hpp"

/**
 * This class stores the sequence number, port number, timestamp, and ip address
 * of the packet header.  This is enough information to uniquely identify each
 * packet.  
 */
class SysMessageID : public Object {
protected:
  //
  // These four fields unique identify a message.
  //

  /** The iMQ sequence number of the packet. */
  PRUint32        sequence;

  /** The transport port number of the packet. */
  PRUint32        port;

  /** The timestamp of the packet. */
  PRUint64        timestamp;

  /** The ip address of the packet.  It is stored in IPv6 format.*/
  IPAddress       ip;


  /** A string representation of the message ID */
  UTF8String *    msgIDStr;

  /** Initializes all member variables */
  void init();

public:
  SysMessageID();

  SysMessageID(const SysMessageID& sysMessageID);
  SysMessageID& operator=(const SysMessageID& sysMessageID);

  ~SysMessageID();

  /**
   * This method resets each field of the packet 
   */
  void reset();

  /**
   * This method reads the ID from the input stream in.
   *
   * @param in is the input stream to read from
   * @returns IMQ_SUCCESS if successful and an error otherwise 
   */
  iMQError readID(IMQDataInputStream * const in);

  /**
   * This method writes the ID to the output stream out.
   *
   * @param out is the output stream to write to
   * @returns IMQ_SUCCESS if successful and an error otherwise 
   */
  iMQError writeID(IMQDataOutputStream * const out) const;

  /**
   * This method returns the iMQ sequence number of the packet.
   *
   * @returns the iMQ sequence number of the packet in host order format.
   */
  PRUint32 getSequence() const;

  /**
   * This method returns the port number of the packet.
   *
   * @returns the port number of the packet in host order format.
   */
  PRUint32 getPort() const;

  /**
   * This method returns the timestamp of the packet.
   *
   * @returns the timestamp of the packet in host order format.
   */
  PRUint64 getTimestamp() const;


  /**
   * This method returns the IP address of the packet in IPv6 format.
   *
   * @param ipv6Addr is the buffer where the address is placed
   * @returns IMQ_SUCCESS if successful and an error otherwise.  
   */
  iMQError getIPv6Address(PRUint8 * const ipv6Addr) const;

  void setSequence(const PRUint32 sequence);
  void setPort(const PRUint32 port);
  void setTimestamp(const PRUint64 timestamp);
  void setIPv6Address(const PRUint8 * const ipv6Addr);
  void setIPv6Address(const IPAddress * const ipv6Addr);

  /**
   * Returns a string representation of the system message ID.
   *
   * @returns a string representation of the system message ID.  NULL
   * is returned if memory cannot be allocated to store the string.  */
  const UTF8String * toString();

  /**
   * Returns true iff id matches this
   *
   * @param id the SysMessageID to compare to
   * @return true iff the two SysMessageID's are equivalent
   */
  PRBool equals(const SysMessageID * const id) const;

};

#endif // SYSMESSAGEID_HPP 




