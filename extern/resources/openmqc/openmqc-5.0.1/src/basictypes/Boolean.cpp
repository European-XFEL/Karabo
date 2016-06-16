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
 * @(#)Boolean.cpp	1.4 06/26/07
 */ 

#include "Boolean.hpp"
#include "../debug/DebugUtils.h"
#include "../util/UtilityMacros.h"

/*
 * Default constructor.
 */
Boolean::Boolean()
{
  CHECK_OBJECT_VALIDITY();

  this->value = BOOLEAN_DEFAULT_VALUE;
}

/*
 * 
 */
Boolean::Boolean(const PRBool valueArg)
{
  CHECK_OBJECT_VALIDITY();

  this->value = valueArg;
}

/*
 * Return a pointer to a deep copy of this object.
 */ 
BasicType *
Boolean::clone() const
{
  CHECK_OBJECT_VALIDITY();

  return new Boolean(this->value);
}

/*
 * Set the value of this object to the value parameter.
 */
void
Boolean::setValue(const PRBool valueArg)
{
  CHECK_OBJECT_VALIDITY();

  this->value = valueArg;
}

/*
 * Return the value of this object.
 */
PRBool
Boolean::getValue() const
{
  CHECK_OBJECT_VALIDITY();

  return this->value;
}

/*
 * Return the type of this object.
 */
TypeEnum
Boolean::getType() const
{
  CHECK_OBJECT_VALIDITY();

  return BOOLEAN_TYPE;
}

/*
 * Read the value of the object from the input stream.
 *
 * Return an error if the read fails.
 */
iMQError 
Boolean::read(IMQDataInputStream * const in)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( in );

  RETURN_IF_ERROR( in->readBoolean(&this->value) );
  
  return IMQ_SUCCESS;
}

/*
 * Write the value of the object to the output stream.
 *
 * Return an error if the write fails.
 */
iMQError 
Boolean::write(IMQDataOutputStream * const out) const
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( out );

  RETURN_IF_ERROR( out->writeBoolean(this->value) );
  
  return IMQ_SUCCESS;
}

/*
 * Print a string representation the value of the object to the file.
 */
iMQError 
Boolean::print(FILE * const file) const
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( file );

  PRInt64 bytesWritten = fprintf(file, "%s", this->value ? "true" : "false");
  RETURN_ERROR_IF( bytesWritten <= 0, IMQ_FILE_OUTPUT_ERROR );
  
  return IMQ_SUCCESS;
}

/*
 *
 */
PRBool       
Boolean::equals(const BasicType * const object) const
{
  CHECK_OBJECT_VALIDITY();

  return ((object != NULL)                             &&
          (object->getType() == this->getType())       &&
          (((Boolean*)object)->getValue() == this->value));
}

/*
 * Returns a 32-bit hash code for this number.  
 */
PLHashNumber
Boolean::hashCode() const
{
  CHECK_OBJECT_VALIDITY();

  return this->value;
}


/*
 *
 */
const char *
Boolean::toString()
{
  CHECK_OBJECT_VALIDITY();

  if (this->value) {
    return "TRUE";
  } else {
    return "FALSE";
  }
}


/*
 *
 */
iMQError
Boolean::getBoolValue(PRBool * const valueArg) const
{
  RETURN_ERROR_IF_NULL( valueArg );
  *valueArg = this->value;

  return IMQ_SUCCESS;
}

/*
 *
 */
iMQError
Boolean::getStringValue(const char ** const valueArg) const
{
  RETURN_ERROR_IF_NULL( valueArg );
  *valueArg = ((Boolean*)this)->toString();

  return IMQ_SUCCESS;
}
