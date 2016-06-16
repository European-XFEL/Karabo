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
 * @(#)Serialize.cpp	1.3 06/26/07
 */ 

#include "Serialize.hpp"
#include "../util/UtilityMacros.h"
#include "../basictypes/AllBasicTypes.hpp"

/**
 *
 */
iMQError 
Serialize::serialIDToType(const PRUint64 serialID, 
                          TypeEnum * const classType)
{
  RETURN_ERROR_IF_NULL(classType);  

  if (LL_EQ( serialID, SERIALIZE_HASHTABLE_SERIAL_ID )) {
    *classType = HASHTABLE_TYPE;
  }
  else if (LL_EQ( serialID, SERIALIZE_BOOLEAN_SERIAL_ID )) {
    *classType = BOOLEAN_TYPE;
  }
  else if (LL_EQ( serialID, SERIALIZE_BYTE_SERIAL_ID )) {
    *classType = BYTE_TYPE;
  }
  else if (LL_EQ( serialID, SERIALIZE_SHORT_SERIAL_ID )) {
    *classType = SHORT_TYPE;
  }
  else if (LL_EQ( serialID, SERIALIZE_INTEGER_SERIAL_ID )) {
    *classType = INTEGER_TYPE;
  }
  else if (LL_EQ( serialID, SERIALIZE_LONG_SERIAL_ID )) {
    *classType = LONG_TYPE;
  }
  else if (LL_EQ( serialID, SERIALIZE_FLOAT_SERIAL_ID )) {
    *classType = FLOAT_TYPE;
  }
  else if (LL_EQ( serialID, SERIALIZE_DOUBLE_SERIAL_ID )) {
    *classType = DOUBLE_TYPE;
  }
  else if (LL_EQ( serialID, SERIALIZE_NUMBER_SERIAL_ID )) {
    *classType = NUMBER_TYPE;
  }
  else {
	*classType = UNKNOWN_TYPE;
    return IMQ_SERIALIZE_BAD_CLASS_UID;
  }

  return IMQ_SUCCESS;
}

/**
 *
 */
iMQError 
Serialize::typeToSerialID(const TypeEnum   classType,
                          PRUint64 * const serialID)
{
  RETURN_ERROR_IF_NULL( serialID );  

  ASSERT( (classType >= BOOLEAN_TYPE) && (classType <= NULL_TYPE) );
  *serialID = SERIAL_ID_BY_TYPE[classType];

  return IMQ_SUCCESS;
}


/**
 *
 */
iMQError
Serialize::classTypeToClassDescBytes(const TypeEnum classType, 
                                     PRUint8 const ** const classDesc, 
                                     PRUint32 * const classDescLen)
{
  RETURN_ERROR_IF_NULL( classDesc );  
  RETURN_ERROR_IF_NULL( classDescLen );  
  ASSERT( (classType >= BOOLEAN_TYPE) && (classType <= NULL_TYPE) );

  if (CLASS_DESC_BY_TYPE[classType] == NULL) {
	*classDesc = NULL;
	*classDescLen = 0;
    return IMQ_SERIALIZE_UNRECOGNIZED_CLASS; 
  }

  *classDesc = CLASS_DESC_BY_TYPE[classType];
  *classDescLen = CLASS_DESC_SIZE_BY_TYPE[classType];

  return IMQ_SUCCESS;
}



/**
 * Factory for new objects
 */
iMQError
Serialize::createObject(const TypeEnum classType, 
                        BasicType ** const object)
{
  RETURN_ERROR_IF_NULL( object );
  *object = NULL;
  
  // Allocate a new object of classType
  BasicType * newObject = NULL;
  switch (classType) {

  case BOOLEAN_TYPE:
    newObject = new Boolean();
    break;

  case BYTE_TYPE:
    newObject = new Byte();
    break;

  case SHORT_TYPE:
    newObject = new Short();
    break;

  case INTEGER_TYPE:
    newObject = new Integer();
    break;

  case LONG_TYPE:
    newObject = new Long();
    break;

  case FLOAT_TYPE:
    newObject = new Float();
    break;

  case DOUBLE_TYPE:
    newObject = new Double();
    break;

  case UTF8_STRING_TYPE:
    newObject = new UTF8String();
    break;

  case UTF8_LONG_STRING_TYPE:
    PRBool isLongString;
    isLongString = PR_TRUE;
    newObject = new UTF8String(isLongString);
    break;

  default:
    return IMQ_SERIALIZE_UNRECOGNIZED_CLASS;
  }

  RETURN_IF_OUT_OF_MEMORY( newObject );

  *object = newObject;
  return IMQ_SUCCESS;
}




