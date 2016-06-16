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
 * @(#)Properties.cpp	1.6 06/26/07
 */ 

#include <float.h>
#include "Properties.hpp"
#include "../util/UtilityMacros.h"
#include "../basictypes/AllBasicTypes.hpp"

/*
 *
 */
Properties::Properties(): HandledObject()
{
  CHECK_OBJECT_VALIDITY();
}

Properties::Properties(PRBool lazy) : HandledObject(lazy)
{
  CHECK_OBJECT_VALIDITY();
}

/*
 *
 */
Properties::Properties(const Properties& properties) : HandledObject(),
   hashtable(properties.hashtable)
{
  CHECK_OBJECT_VALIDITY();
}

/*
 *
 */
Properties::~Properties()
{
  CHECK_OBJECT_VALIDITY();
}

void
Properties::reset()
{
  hashtable.reset();
}
/*
 *
 */
Properties*
Properties::clone() const
{
  CHECK_OBJECT_VALIDITY();

  Properties * clonedProps = new Properties(*this);
  if (clonedProps == NULL) {
      return NULL;
  }
  if (clonedProps->getInitializationError() != IMQ_SUCCESS) {
    DELETE( clonedProps );
    clonedProps = NULL;
  }
  return clonedProps;
}

/*
 *
 */
iMQError 
Properties::getPropertyType(const char *     const propertyName,
                            TypeEnum * const propertyType) const
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );
  RETURN_ERROR_IF_NULL( propertyType );
  *propertyType = UNKNOWN_TYPE;

  // Get the property
  const BasicType *  value = NULL;
  UTF8String prop(propertyName);
  RETURN_IF_ERROR( this->getBasicTypeProperty(&prop, &value) );

  *propertyType = value->getType();

  return IMQ_SUCCESS;
}

/*
 *
 */
iMQError 
Properties::getStringProperty(const char * const  propertyName,
                              const char **       propertyValue) const
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );
  RETURN_ERROR_IF_NULL( propertyValue );
  *propertyValue = NULL;

  // Look up the value based on the key
  const BasicType *  value = NULL;
  UTF8String prop(propertyName);
  RETURN_IF_ERROR( this->getBasicTypeProperty(&prop, &value) );
  RETURN_IF_ERROR( value->getStringValue(propertyValue) );

  return IMQ_SUCCESS;
}

/*
 *
 */
iMQError 
Properties::setStringProperty(const char * const propertyName,
                              const char * const propertyValue)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );
  RETURN_ERROR_IF_NULL( propertyValue ); 

  return setBasicTypeProperty(new UTF8String(propertyName),
                              new UTF8String(propertyValue));
}


/*
 *
 */
iMQError 
Properties::setUTF8StringProperty(UTF8String * const propertyName,
                                  UTF8String * const propertyValue)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );
  RETURN_ERROR_IF_NULL( propertyValue ); 

  return setBasicTypeProperty(propertyName, propertyValue);
}


/*
 * The caller should not reference propertyValue after this method returns. 
 * setBasicTypeProperty is responsible for deleting propertyValue whether
 * this function succeeds or not.
 */
iMQError 
Properties::setBasicTypeProperty( UTF8String * propertyName,
                                  BasicType  * propertyValue)
{

  CHECK_OBJECT_VALIDITY();

  // Make sure that propertyName and propertyValue are valid
  if ((propertyName == NULL) || (propertyValue == NULL)) {
    DELETE( propertyName );
    DELETE( propertyValue );
    RETURN_UNEXPECTED_ERROR( IMQ_NULL_PTR_ARG );
  }

  // Add the <Name,Value> pair to the hash.  Delete the name and value
  // if the call is unsuccessful, otherwise BasicTypeHashTable::
  // reset() will delete them.
  iMQError error = hashtable.addEntry(propertyName, propertyValue);
  if (error != IMQ_SUCCESS) {
    DELETE( propertyName );
    DELETE( propertyValue );
    return error;
  }

  return IMQ_SUCCESS;
}

/*
 *
 */
iMQError 
Properties::getBasicTypeProperty(const UTF8String *  const propertyName,
                                 const BasicType  ** const propertyValue) const
{
  CHECK_OBJECT_VALIDITY();

  // Make sure that propertyName and propertyValue are valid
  if ((propertyName == NULL) || (propertyValue == NULL)) {
    RETURN_UNEXPECTED_ERROR( IMQ_NULL_PTR_ARG );
  }
  RETURN_ERROR_IF_NULL( propertyValue );

  // Lookup the value based on propertyName 
  iMQError error = hashtable.getValueFromKey(propertyName, 
                                             (const Object**)&(*propertyValue));
  if (error != IMQ_SUCCESS) {
    return error;
  }

  return IMQ_SUCCESS;
}


/*
 *
 */
iMQError 
Properties::setBooleanProperty(const char     * const propertyName,
                               const PRBool           propertyValue)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );

  return setBasicTypeProperty(new UTF8String(propertyName),
                              new Boolean(propertyValue));
}

/*
 *
 */
iMQError 
Properties::getBooleanProperty(const char   * const propertyName,
                                     PRBool * const propertyValue) const
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );
  RETURN_ERROR_IF_NULL( propertyValue );

  const BasicType *  value = NULL;
  UTF8String prop(propertyName);
  RETURN_IF_ERROR( this->getBasicTypeProperty(&prop, &value) );
  RETURN_IF_ERROR( value->getBoolValue(propertyValue) );
  
  return IMQ_SUCCESS;
}

/*
 *
 */
iMQError 
Properties::setByteProperty(const char     * const propertyName,
                            const PRInt8           propertyValue)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );

  return setBasicTypeProperty(new UTF8String(propertyName),
                              new Byte(propertyValue));
}

/*
 *
 */
iMQError 
Properties::getByteProperty(const char    * const propertyName,
                                  PRInt8  * const propertyValue) const
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );
  RETURN_ERROR_IF_NULL( propertyValue );

  const BasicType *  value = NULL;
  UTF8String prop(propertyName);
  RETURN_IF_ERROR( this->getBasicTypeProperty(&prop, &value) );
  RETURN_IF_ERROR( value->getInt8Value(propertyValue) );
  
  return IMQ_SUCCESS;
}

/*
 *
 */
iMQError 
Properties::setShortProperty(const char     * const propertyName,
                             const PRInt16          propertyValue)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );

  return setBasicTypeProperty(new UTF8String(propertyName),
                              new Short(propertyValue));
}

/*
 *
 */
iMQError 
Properties::getShortProperty(const char     * const propertyName,
                                   PRInt16  * const propertyValue) const
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );
  RETURN_ERROR_IF_NULL( propertyValue );

  const BasicType *  value = NULL;
  UTF8String prop(propertyName);
  RETURN_IF_ERROR( this->getBasicTypeProperty(&prop, &value) );
  RETURN_IF_ERROR( value->getInt16Value(propertyValue) );
  
  return IMQ_SUCCESS;
}

/*
 *
 */
iMQError 
Properties::setIntegerProperty(const char     * const propertyName,
                               const PRInt32          propertyValue)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );

  return setBasicTypeProperty(new UTF8String(propertyName),
                              new Integer(propertyValue));
}

/*
 *
 */
iMQError 
Properties::getIntegerProperty(const char     * const propertyName,
                                     PRInt32  * const propertyValue) const
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );
  RETURN_ERROR_IF_NULL( propertyValue );

  const BasicType *  value = NULL;
  UTF8String prop(propertyName);
  RETURN_IF_ERROR( this->getBasicTypeProperty(&prop, &value) );
  RETURN_IF_ERROR( value->getInt32Value(propertyValue) );
  
  return IMQ_SUCCESS;
}


/*
 *
 */
iMQError 
Properties::setLongProperty(const char     * const propertyName,
                            const PRInt64          propertyValue)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );

  return setBasicTypeProperty(new UTF8String(propertyName),
                              new Long(propertyValue));
}

/*
 *
 */
iMQError 
Properties::getLongProperty(const char     * const propertyName,
                                  PRInt64  * const propertyValue) const
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );
  RETURN_ERROR_IF_NULL( propertyValue );

  const BasicType *  value = NULL;
  UTF8String prop(propertyName);
  RETURN_IF_ERROR( this->getBasicTypeProperty(&prop, &value) );
  RETURN_IF_ERROR( value->getInt64Value(propertyValue) );
  
  return IMQ_SUCCESS;
}


/*
 *
 */
iMQError 
Properties::setFloatProperty(const char     * const propertyName,
                             const PRFloat32        propertyValue)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );

  return setBasicTypeProperty(new UTF8String(propertyName),
                              new Float(propertyValue));
}

/*
 *
 */
iMQError 
Properties::getFloatProperty(const char      * const propertyName,
                                   PRFloat32 * const propertyValue) const
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );
  RETURN_ERROR_IF_NULL( propertyValue );

  const BasicType *  value = NULL;
  UTF8String prop(propertyName);
  RETURN_IF_ERROR( this->getBasicTypeProperty(&prop, &value) );
  RETURN_IF_ERROR( value->getFloat32Value(propertyValue) );
  
  return IMQ_SUCCESS;
}


/*
 *
 */
iMQError 
Properties::setDoubleProperty(const char     * const propertyName,
                              const PRFloat64        propertyValue)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );

  return setBasicTypeProperty(new UTF8String(propertyName),
                              new Double(propertyValue));
}

/*
 *
 */
iMQError 
Properties::getDoubleProperty(const char      * const propertyName,
                                    PRFloat64 * const propertyValue) const
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );
  RETURN_ERROR_IF_NULL( propertyValue );

  const BasicType *  value = NULL;
  UTF8String prop(propertyName);
  RETURN_IF_ERROR( this->getBasicTypeProperty(&prop, &value) );
  RETURN_IF_ERROR( value->getFloat64Value(propertyValue) );
  
  return IMQ_SUCCESS;
}

/*
 *
 */
iMQError
Properties::removeProperty(const char * propertyName)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( propertyName );
  const BasicType *  value = NULL;
  UTF8String prop(propertyName);

  RETURN_IF_ERROR( hashtable.removeEntry(&prop) );
  
  return IMQ_SUCCESS;
}


/*
 *
 */
iMQError  
Properties::keyIterationStart()
{
  return hashtable.keyIterationStart();
}

/*
 *
 */
PRBool    
Properties::keyIterationHasNext()
{
  return hashtable.keyIterationHasNext();
}

/*
 *
 */
iMQError
Properties::keyIterationGetNext(const char ** const key)
{
  iMQError errorCode = IMQ_SUCCESS;

  // Get the next key
  const BasicType * basicTypeKey = NULL;
  errorCode = hashtable.keyIterationGetNext(&basicTypeKey);
  if (errorCode == IMQ_SUCCESS) {
    if ((basicTypeKey->getType() == UTF8_STRING_TYPE) ||
        (basicTypeKey->getType() == UTF8_LONG_STRING_TYPE))
    {
      const UTF8String * keyString = (const UTF8String*)basicTypeKey;
      *key = keyString->getCharStr();
    }
  }

  return errorCode;
}

/*
 *
 */
iMQError
Properties::print(FILE * const out)
{
  return hashtable.print(out);
}

/*
 *
 */
iMQError  
Properties::getNumKeys(PRUint32 * const numKeys) const
{
  return hashtable.getNumKeys(numKeys);
}

const char * 
Properties::toString(const char * const linePrefix)
{
  return hashtable.toString(linePrefix);
}

/*
 *
 */
BasicTypeHashtable *
Properties::getHashtable()
{
  return &hashtable;
}

// To implement HandledObject
HandledObjectType 
Properties::getObjectType() const
{
  return PROPERTIES_OBJECT;
}



// Read properties from file
//
// File looks like.  (values cannot have spaces)
// attr1 = value1
// attr2 = value2
// attr3 = value3
//
iMQError
Properties::readFromFile(const char * const fileName)
{
  iMQError errorCode = IMQ_SUCCESS;
  FILE * input = NULL;
  NULLCHK( fileName );
  char propertyName[PROPERTIES_MAX_STRING_SIZE];
  char propertyValue[PROPERTIES_MAX_STRING_SIZE];

  input = fopen(fileName, "r");
  CNDCHK( input == NULL, IMQ_FILE_NOT_FOUND );

  // Read in properties until we get to the end of the file
  while (!feof(input)) {
    // read in the property name and value
    int valuesRead = 0;
    valuesRead = fscanf( input, " %s = %[^\n] ", propertyName, propertyValue );
    if (valuesRead == -1) {
      break;
    }
    CNDCHK( valuesRead != 2, IMQ_PROPERTY_FILE_ERROR );
    
    // set the property
    ERRCHK( this->setStringProperty(propertyName, propertyValue) );
  }

  fclose(input);
  return IMQ_SUCCESS;

 Cleanup:
  if (input != NULL) {
    fclose(input);
  }
  return errorCode;
}
