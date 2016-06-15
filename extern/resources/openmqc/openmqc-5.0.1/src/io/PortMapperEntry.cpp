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
 * @(#)PortMapperEntry.cpp	1.4 06/26/07
 */ 

#include "PortMapperEntry.hpp"
#include "../util/UtilityMacros.h"
#include "PortMapperTable.hpp"

/*
 *
 */
PortMapperEntry::PortMapperEntry()
{
  CHECK_OBJECT_VALIDITY();

  port     = 0;
  protocol = NULL;
  type     = NULL;
  name     = NULL;
}

/*
 *
 */
PortMapperEntry::~PortMapperEntry()
{
  CHECK_OBJECT_VALIDITY();

  ASSERT( ((protocol == NULL) && (type == NULL) && (name == NULL) && (port == 0)) ||
          ((protocol != NULL) && (type != NULL) && (name != NULL)) );

  reset();
}

/*
 *
 */
void
PortMapperEntry::reset()
{
  CHECK_OBJECT_VALIDITY();

  port = 0;
  DELETE( protocol );
  DELETE( type );
  DELETE( name );
}

/*
 *
 */
iMQError 
PortMapperEntry::parse(const UTF8String * const serviceLine)
{
  CHECK_OBJECT_VALIDITY();

  ObjectVector *  fieldVector = NULL;
  UTF8String *    portStr     = NULL;
  iMQError        errorCode   = IMQ_SUCCESS;
  RETURN_ERROR_IF_NULL( serviceLine );

  // serviceLine should look like
  // <service name><SP><protocol><SP><type><SP><port>
  ERRCHK( serviceLine->tokenize(PORTMAPPER_FIELD_SEPARATOR, &fieldVector) );  
  /*
   * Check that serviceLine contains at least 4 elements.
   * Each line is of the form:
   *  portmapper tcp PORTMAPPER 7676 [key1=value1,key2=value2,...]
   * The C API only needs the first 4 elements. As of now, it will not parse the 
   * optional properties section encapsulated with "[" and "]".
   * Note: The keys/values may contain spaces.
   */
  CNDCHK( (fieldVector->size() < PORTMAPPER_SERVICE_NUM_FIELDS),
          IMQ_PORTMAPPER_INVALID_INPUT );  
  ERRCHK( fieldVector->remove(0, (void**)&(this->name)) );
  ERRCHK( fieldVector->remove(0, (void**)&(this->protocol)) );
  ERRCHK( fieldVector->remove(0, (void**)&(this->type)) );
  ERRCHK( fieldVector->remove(0, (void**)&portStr) );
  ERRCHK( portStr->getUint16Value(&(this->port)) );

  DELETE( fieldVector );
  DELETE( portStr );

  return IMQ_SUCCESS;

 Cleanup:
  DELETE( fieldVector );
  DELETE( portStr );
  reset();
  
  return errorCode;
}

/*
 *
 */
const UTF8String *
PortMapperEntry::getName() const
{
  CHECK_OBJECT_VALIDITY();

  return name;
}

/*
 *
 */
PRUint16 
PortMapperEntry::getPort() const
{
  CHECK_OBJECT_VALIDITY();

  return port;
}


