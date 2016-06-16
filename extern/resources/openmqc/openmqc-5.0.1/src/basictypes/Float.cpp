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
 * @(#)Float.cpp	1.5 06/26/07
 */ 

#include "Float.hpp"
#include "../debug/DebugUtils.h"
#include "../util/UtilityMacros.h"
#include <memory.h>

/*
 * Default constructor.
 */
Float::Float()
{
  CHECK_OBJECT_VALIDITY();

  this->value    = FLOAT_DEFAULT_VALUE;
  this->valueStr = NULL;
}

/*
 * 
 */
Float::Float(const PRFloat32 valueArg)
{
  CHECK_OBJECT_VALIDITY();

  this->value    = valueArg;
  this->valueStr = NULL;
}

/*
 * 
 */
Float::~Float()
{
  CHECK_OBJECT_VALIDITY();

  DELETE_ARR( this->valueStr );
}


/*
 * Return a pointer to a deep copy of this object.
 */
BasicType *
Float::clone() const
{
  CHECK_OBJECT_VALIDITY();

  return new Float(this->value);
}

/*
 * Set the value of this object to the value parameter.
 */
void
Float::setValue(const PRFloat32 valueArg)
{
  CHECK_OBJECT_VALIDITY();

  this->value = valueArg;
}

/*
 * Return the value of this object.
 */
PRFloat32
Float::getValue() const
{
  CHECK_OBJECT_VALIDITY();

  return this->value;
}

/*
 * Return the type of this object.
 */
TypeEnum
Float::getType() const
{
  CHECK_OBJECT_VALIDITY();

  return FLOAT_TYPE;
}

/*
 * Read the value of the object from the input stream.
 *
 * Return an error if the read fails.
 */
iMQError 
Float::read(IMQDataInputStream * const in)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( in );

  RETURN_IF_ERROR( in->readFloat32(&this->value) );
  
  return IMQ_SUCCESS;
}

/*
 * Write the value of the object to the output stream.
 *
 * Return an error if the write fails.
 */
iMQError 
Float::write(IMQDataOutputStream * const out) const
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( out );

  RETURN_IF_ERROR( out->writeFloat32(this->value) );
  
  return IMQ_SUCCESS;
}

/*
 * Print the value of the object to the file.
 *
 * Return an error if the print fails
 */
iMQError 
Float::print(FILE * const file) const
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( file );

  PRInt32 bytesWritten = fprintf(file, "%g", (PRFloat64)this->value);
  RETURN_ERROR_IF( bytesWritten <= 0, IMQ_FILE_OUTPUT_ERROR );
  
  return IMQ_SUCCESS;
}

/*
 *
 */
PRBool       
Float::equals(const BasicType * const object) const
{
  CHECK_OBJECT_VALIDITY();

  return ((object != NULL)                          &&
          (object->getType() == this->getType())    &&
          (((Float*)object)->getValue() == this->value));
}

/*
 * Returns a 32-bit hash code for this number.
 */
PLHashNumber
Float::hashCode() const
{
  CHECK_OBJECT_VALIDITY();

  PLHashNumber hashCode = 0;
  memcpy((void*)&hashCode, 
         (void*)&this->value, 
         MIN( sizeof(hashCode), sizeof(this->value) ));

  return hashCode;
}


/*
 * Return a char* representation of this object.
 */
const char *
Float::toString()
{
  CHECK_OBJECT_VALIDITY();

  if (this->valueStr != NULL) {
    return this->valueStr;
  } 
  this->valueStr = new char[FLOAT_MAX_STR_SIZE];
  if (this->valueStr == NULL) {
    return "<out-of-memory>";
  }

  SNPRINTF(this->valueStr, FLOAT_MAX_STR_SIZE, "%g", (PRFloat64)this->value);
  // Just to be safe.  snprintf won't automatically null terminate for us.
  this->valueStr[FLOAT_MAX_STR_SIZE-1] = '\0'; 

  return this->valueStr;
}



/*
 *
 */
iMQError
Float::getFloat32Value(PRFloat32 * const valueArg) const
{
  RETURN_ERROR_IF_NULL( valueArg );
  *valueArg = this->value;

  return IMQ_SUCCESS;
}

/*
 *
 */
iMQError
Float::getFloat64Value(PRFloat64 * const valueArg) const
{
  RETURN_ERROR_IF_NULL( valueArg );
  *valueArg = this->value;

  return IMQ_SUCCESS;
}

/*
 *
 */
iMQError
Float::getStringValue(const char ** const valueArg) const
{
  RETURN_ERROR_IF_NULL( valueArg );
  *valueArg = ((Float*)this)->toString();

  return IMQ_SUCCESS;
}


