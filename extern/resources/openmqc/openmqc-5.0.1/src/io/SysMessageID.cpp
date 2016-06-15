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
 * @(#)SysMessageID.cpp	1.6 06/26/07
 */ 

#include "SysMessageID.hpp"
#include "../debug/DebugUtils.h"
#include "../util/UtilityMacros.h"

#include <prlong.h>


/*
 * Construct an unititialized system message ID. It is assumed
 * the caller will set the fields either explicitly or via
 * readID()
 */
SysMessageID::SysMessageID()
{
  CHECK_OBJECT_VALIDITY();

  this->init();
}

/*
 *
 */
SysMessageID::SysMessageID(const SysMessageID& sysMessageID)
{
  CHECK_OBJECT_VALIDITY();
  this->init();

  this->sequence  = sysMessageID.sequence;
  this->port      = sysMessageID.port;
  this->timestamp = sysMessageID.timestamp;
  this->ip        = sysMessageID.ip;
  this->msgIDStr  = NULL;
}

/*
 *
 */
SysMessageID& 
SysMessageID::operator=(const SysMessageID& sysMessageID)
{
  CHECK_OBJECT_VALIDITY();

  this->reset();

  this->sequence  = sysMessageID.sequence;
  this->port      = sysMessageID.port;
  this->timestamp = sysMessageID.timestamp;
  this->ip        = sysMessageID.ip;
  this->msgIDStr  = NULL;

  return *this;
}

/*
 *
 */
SysMessageID::~SysMessageID()
{
  CHECK_OBJECT_VALIDITY();

  reset();
}

/*
 *
 */
void
SysMessageID::init()
{
  sequence     = 0;
  port         = 0;
  timestamp    = 0;
  msgIDStr     = NULL;
}

/*
 * Clears the message id
 */
void
SysMessageID::reset()
{
  CHECK_OBJECT_VALIDITY();
  ip.reset();
  DELETE( msgIDStr );
  init();
}

/*
 * 
 */
PRUint32
SysMessageID::getSequence() const
{
  CHECK_OBJECT_VALIDITY();

  return sequence;
}

/*
 * 
 */
PRUint32
SysMessageID::getPort() const
{
  CHECK_OBJECT_VALIDITY();

  return port;
}

/*
 * 
 */
PRUint64
SysMessageID::getTimestamp() const
{
  CHECK_OBJECT_VALIDITY();

  return timestamp;
}


/*
 * Returns the IPv6 address
 */
iMQError
SysMessageID::getIPv6Address(PRUint8 * const ipv6Addr) const
{
  CHECK_OBJECT_VALIDITY();

  return ip.getIPv6Address(ipv6Addr);
}

/*
 * 
 */
void
SysMessageID::setSequence(const PRUint32 sequenceArg) 
{
  CHECK_OBJECT_VALIDITY();

  this->sequence = sequenceArg;
}

/*
 * 
 */
void
SysMessageID::setPort(const PRUint32 portArg) 
{
  CHECK_OBJECT_VALIDITY();

  this->port = portArg;
}

/*
 * 
 */
void
SysMessageID::setTimestamp(const PRUint64 timestampArg)
{
  CHECK_OBJECT_VALIDITY();

  this->timestamp = timestampArg;
}

/*
 * Sets the IPv6 address
 */
void
SysMessageID::setIPv6Address(const PRUint8 * const ipv6Addr) 
{
  CHECK_OBJECT_VALIDITY();

  ip.setAddressFromIPv6Address(ipv6Addr);
}

/*
 * Sets the IPv6 address
 */
void
SysMessageID::setIPv6Address(const IPAddress * const ipv6Addr) 
{
  CHECK_OBJECT_VALIDITY();

  if (ipv6Addr != NULL) {
    this->ip = *ipv6Addr;
  }
}

/*
 * 
 */
iMQError
SysMessageID::readID(IMQDataInputStream * const in)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( in );
  reset();

  //
  // Read in the timestamp, ip address, port, and sequence
  //
  RETURN_IF_ERROR( in->readUint64(&(this->timestamp)) );
  RETURN_IF_ERROR( ip.readAddress(in) );
  RETURN_IF_ERROR( in->readUint32(&(this->port)) );
  RETURN_IF_ERROR( in->readUint32(&(this->sequence)) );

  return IMQ_SUCCESS;
}


/*
 * 
 */
iMQError
SysMessageID::writeID(IMQDataOutputStream * const out) const
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( out );

  //
  // Write in the timestamp, ip address, port, and sequence
  //
  RETURN_IF_ERROR( out->writeUint64(this->timestamp) );
  RETURN_IF_ERROR( ip.writeAddress(out) );
  RETURN_IF_ERROR( out->writeUint32(this->port) );
  RETURN_IF_ERROR( out->writeUint32(this->sequence) );
  
  return IMQ_SUCCESS;
}



/*
 *
 */
const int MAX_ID_STR_LEN = 1000;
const UTF8String * 
SysMessageID::toString()
{
  CHECK_OBJECT_VALIDITY();

  if (msgIDStr != NULL) {
    return msgIDStr;
  }

  char idStr[MAX_ID_STR_LEN];

  int bytesWritten = 0;
  bytesWritten = PR_snprintf( idStr, sizeof(idStr), "ID:%u-%s-%u-%lld", 
                                                 sequence,
                                                 ip.toCharStr(),
                                                 port,
                                                 timestamp );
  msgIDStr = new UTF8String(idStr);
  return msgIDStr;
}






/*
 *
 */
PRBool 
SysMessageID::equals(const SysMessageID * const id) const
{
  CHECK_OBJECT_VALIDITY();

  if (id == NULL) {
    return PR_FALSE;
  }

  return 
    (this->sequence == id->sequence)        &&
    (this->port     == id->port)            &&
    (LL_EQ(this->timestamp, id->timestamp)) &&
    (this->ip.equals(&(id->ip)));
}
