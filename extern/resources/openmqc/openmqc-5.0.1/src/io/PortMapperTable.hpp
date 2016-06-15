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
 * @(#)PortMapperTable.hpp	1.5 06/28/07
 */ 

#ifndef PORTMAPPERTABLE_H
#define PORTMAPPERTABLE_H

#include "../debug/DebugUtils.h"
#include "../error/ErrorCodes.h"
#include "../basictypes/AllBasicTypes.hpp"
#include "../containers/BasicTypeHashtable.hpp"
#include "PortMapperEntry.hpp"
#include "../basictypes/Object.hpp"

/** String that delimits the service lines in the output of the port mapper */
static const char *   PORTMAPPER_LINE_SEPARATOR       = "\n";

/** String that delimits the fields of a single service line */
static const char *   PORTMAPPER_FIELD_SEPARATOR      = " ";

/** The number of fields in the version line of the port mapper service.  An
 *  example of the version line is "101 jmqbroker 2.0" */
static const PRUint32 PORTMAPPER_VERSION_NUM_FIELDS   = 3;

/** The number of fields in the version line of the port mapper service.  An
    example of a service line is "jms tcp NORMAL 59510"*/
static const PRUint32 PORTMAPPER_SERVICE_NUM_FIELDS   = 4;

/** The version of the port mapper service that this table speaks. */
static const char *   PORTMAPPER_VERSION              = "101";
static const char *   PORTMAPPER_VERSION_LINE         = "101\n";

/** The string used to terminate the port service mapper output. */
static const char *   PORTMAPPER_SERVICE_TERMINATOR   = ".";

/**
 * This class encapsulates information about a service. For use with
 * the PortMapper.
 * 
 * @see PortMapperEntry
 * @see PortMapperClient
 */
class PortMapperTable : public Object {
private:
  /** Broker instance name that is returned by by the portmapper server. */
  UTF8String * brokerInstance;

  /** Broker version that is returned by by the portmapper server. */
  UTF8String * brokerVersion;

  /** The portmapper version that is returned by by the port mapper */
  UTF8String * version;

  /**
   * The table that maps the name of the service to information about
   * it, including port number.  
   */
  BasicTypeHashtable serviceTable;

  void init();

  /**
   * This method adds portMapperEntry to the service table.
   *
   * @param portMapperEntry is the entry to add to the service.
   * @returns IMQ_SUCCESS if successful and an error otherwise.
   */
  iMQError add(PortMapperEntry * const portMapperEntry);

public:
  PortMapperTable();
  ~PortMapperTable();

  /**
   * This method resets the PortMapperTable and deallocates all memory
   * associated with it.  
   */
  void reset();

  /**
   * This method parses portServerOutput and populates the portmapper table
   * based on the services and ports represented by portServerOutput.  (These
   * comments were copied from PortMapperTable.java") The format of the data is:
   *
   *  <PRE>
   *  <portmapper version><SP><broker instance name><SP>broker version><NL>
   *  <service name><SP><protocol><SP><type><SP><port><NL>
   *  <.><NL>
   *
   *  Where:
   *
   *  <portmapper version>Portmapper numeric version string (ie "100").
   *  <broker version>    Broker version string (ie "2.0").
   *  <NL>                Newline character (octal 012)
   *  <service name>      Alphanumeric string. No embedded whitespace.
   *  <space>             A single space character
   *  <protocol>          Transport protocol. Typically "tcp" or "ssl"
   *  <service>           Service type. Typically "NORMAL", "ADMIN" or
   *                      "PORTMAPPER"
   *  <port>              Numeric string. Service port number
   *  <.>                 The '.' (dot) character
   *
   *  An example would be:
   *
   *  101 jmqbroker 2.0
   *  portmapper tcp PORTMAPPER 7575
   *  jms tcp NORMAL 59510
   *  admin tcp ADMIN 59997
   *  ssljms ssl NORMAL 42322
   *  .
   *
   *   </PRE>
   *
   * @param portServerOutput  is the string that was received from
   *  port server.  See the source for how this string is formatted.  
   * @returns IMQ_SUCCESS if successful and an error otherwise.
   *  */
  iMQError parse(const UTF8String * const portServerOutput);
  
  /**
   * This method looks up the service with name serviceName, and returns a
   * pointer to the PortMapperEntry associated with it in the output parameter
   * portMapperEntry.
   *
   * @param serviceName is the name of the service to look up
   * @param portMapperEntry is the output parameter where the pointer to the
   *  PortMapperEntry associated with serviceName is placed 
   * @returns IMQ_SUCCESS if successful and an error otherwise
   */
  iMQError get(const UTF8String * serviceName, 
               const PortMapperEntry ** const portMapperEntry) const;

  /**
   * This method looks up the service with name serviceName, and returns a
   * pointer to the PortMapperEntry associated with it in the output parameter
   * portMapperEntry.  The only difference between this method and the previous,
   * is that this method takes char* and the previous a UTF8String*.
   *
   * @param serviceName is the name of the service to look up
   * @param portMapperEntry is the output parameter where the pointer to the
   *  PortMapperEntry associated with serviceName is placed 
   * @returns IMQ_SUCCESS if successful and an error otherwise 
   */
  iMQError get(const char * serviceName, 
               const PortMapperEntry ** const portMapperEntry) const; 

  /**
   * This method looks up the service associated with (protocol, type), and
   * returns a pointer to the PortMapperEntry associated with it in the output
   * parameter portMapperEntry.
   *
   * @param protocol is the protocol to lookup (e.g. "tcp")
   * @param type is the type of the protocol to lookup (e.g. "NORMAL")
   * @param portMapperEntry is the output parameter where the pointer to the
   *  PortMapperEntry associated with (protocol,type) is placed 
   * @returns IMQ_SUCCESS if successful and an error otherwise 
   */
  iMQError getPortForProtocol(const UTF8String * protocol,
                              const UTF8String * type,
                              const PortMapperEntry ** const portMapperEntry);

  /** Static unit test */
  static iMQError test();

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  PortMapperTable(const PortMapperTable& portMapperTable);
  PortMapperTable& operator=(const PortMapperTable& portMapperTable);
};


#endif











