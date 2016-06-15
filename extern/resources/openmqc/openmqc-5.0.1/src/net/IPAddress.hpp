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
 * @(#)IPAddress.hpp	1.4 06/26/07
 */ 

#ifndef IPADDRESS_H
#define IPADDRESS_H

#include <prtypes.h>
#include "../error/ErrorCodes.h"
#include "../io/IMQDataInputStream.hpp"
#include "../io/IMQDataOutputStream.hpp"


/** Size of an IPv4 address in bytes */
static const PRUint32 IP_ADDRESS_IPV4_SIZE = 4;
/** Size of an IPv6 address in bytes */
static const PRUint32 IP_ADDRESS_IPV6_SIZE = 16;
// Longest IPv4 address: 123.456.789.123
// Longest IPv4 + MACaddress: 123.456.789.123(ab:cd:ef:gh:ij:kl)
// Longest IPv6 address: 12AB:FE90:D8B3:12AB:FE90:D8B3:12AB:FE90
static const PRUint32 IP_ADDRESS_MAX_IPV6_ADDR_STR_LEN = 50; // This could be 40

class IPAddress : public Object {
protected:
  //
  // 128 bit buffer to hold the IP address.  We always store the
  // address as an IPv6 address
  //
  PRUint8     ip[IP_ADDRESS_IPV6_SIZE];
  PRUint32    type;

  char strValue[IP_ADDRESS_MAX_IPV6_ADDR_STR_LEN];

public:

  IPAddress();
  IPAddress( const IPAddress& ipAddress );
  IPAddress& operator=(const IPAddress& ipAddress );

  void reset();

  iMQError readAddress( IMQDataInputStream * const in );
  iMQError writeAddress( IMQDataOutputStream * const out ) const;

  iMQError getIPv6Address( PRUint8 * const ipv6Addr ) const;
  iMQError getIPv4AddressAsNetOrderInt( PRUint32 * const ipv4Addr ) const;

  iMQError setIPv4AddressFromNetOrderInt( const PRUint32 ipv4Addr );
  iMQError setAddressFromIPv6Address( const PRUint8 * const ipv6Addr );

  /**
   * returns a char * representation of the IPv6 address.  If the
   * address is an IPv4 address it looks like 123.456.789.123.  If
   * it's an IPv6 address it looks like
   * 12AB:FE90:D8B3:12AB:FE90:D8B3:12AB:FE90.
   *
   * @returns a char * representation of the IPv6 address.  
   */
  const char * toCharStr();

  /**
   * Pseudonym for toCharStr
   */
  const char * toString() const;


  /**
   * Return true iff this IPAddress is equivalent to ipAddr
   *
   * @param ipAddr the ipaddress to compare to
   * @return true iff this IPAddress is equivalent to ipAddr
   */
  PRBool equals(const IPAddress * const ipAddr) const;


private:
  PRBool isIPv4Mapped( const PRUint8 * const addr, const PRUint32 addrLen ) const;
  PRBool isIPv4Mac( const PRUint8 * const addr, const PRUint32 addrLen ) const;

};


#endif // IPADDRESS_H
