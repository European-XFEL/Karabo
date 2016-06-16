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
 * @(#)PortMapperEntry.hpp	1.4 06/28/07
 */ 

#ifndef PORTMAPPERENTRY_H
#define PORTMAPPERENTRY_H

#include "../debug/DebugUtils.h"
#include "../error/ErrorCodes.h"
#include "../basictypes/AllBasicTypes.hpp"
#include "../basictypes/Object.hpp"

#include <nspr.h>

/**
 * This class stores a single mapping from iMQ protocol name to
 * protocol, type, and port.  E.g.  jms tcp NORMAL 59510
 *
 * @see PortMapperTable
 * @see PortMapperClient
 */
class PortMapperEntry : public Object {
public:
  // These fields are declared public so they can be easily read.  They should
  // not be modified.

  /** The port of the service (e.g. 59510) */
  PRUint16     port;
  /** The protocol used for the service (e.g. tcp) */
  UTF8String * protocol;
  /** The type of the service (e.g. NORMAL) */
  UTF8String * type;
  /** The name of the service (e.g. jms) */
  UTF8String * name;

private:
  void reset();
public:
  PortMapperEntry();
  ~PortMapperEntry();

  /**
   * This method initializes this PortMapperEntry by parsing the service
   * described in serviceLine.
   *
   * @param serviceLine is a string representing the service line with the \n
   * already stripped.  (e.g. "jms tcp NORMAL 59510")
   *
   * @returns IMQ_SUCCESS if successful and an error otherwise 
   */
  iMQError parse(const UTF8String * const serviceLine);
  
  /**
   * This method returns the name of the service.  The caller should not attempt
   * to modify or free the string.
   *
   * @returns the name of the service e.g. "jms".  If the PortMapperEntry has
   * not been initialized, then NULL is returned.  
   */
  const UTF8String * getName() const;

  /**
   * This method returns the port associated with this service.
   *
   * @returns  the port associated with this service.
   */
  PRUint16 getPort() const;
};

#endif // PORTMAPPERENTRY_H






