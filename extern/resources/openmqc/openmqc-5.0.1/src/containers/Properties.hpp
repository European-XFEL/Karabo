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
 * @(#)Properties.hpp	1.5 06/26/07
 */ 

#ifndef PROPERTIES_HPP
#define PROPERTIES_HPP


#include "BasicTypeHashtable.hpp"
#include "../util/PRTypesUtils.h"
#include "../basictypes/AllBasicTypes.hpp"
#include "../basictypes/HandledObject.hpp"

#include <stdio.h>


static const PRInt32 PROPERTIES_MAX_STRING_SIZE = USHRT_MAX;


/** This class maintains a mapping from strings to BasicType's.  It
 *  delegates most responsibility to BasicTypeHashtable.  */
class Properties : public HandledObject
{
private:
  /** A hashtable that stores all of the properties.*/
  BasicTypeHashtable hashtable;

  /** Sets the value of the propertyName property to propertyValue. */
  iMQError setBasicTypeProperty( UTF8String * propertyName,
                                 BasicType  * propertyValue);

  /** Returns the value of the propertyName property in propertyValue */
  iMQError getBasicTypeProperty(const UTF8String *  const propertyName,
                                const BasicType  ** const propertyValue) const;


  /** This currently isn't called from anywhere, so it is made private.
   *  It could be made public, however. */
  iMQError setUTF8StringProperty(UTF8String * const propertyName,
                                 UTF8String * const propertyValue);


public:
  Properties();
  Properties(PRBool lazy);
  Properties(const Properties& properties);
  virtual ~Properties();

  void reset();

  /** Returns the type of the propertyName property in propertyType */
  iMQError getPropertyType(const char *     const propertyName,
                                 TypeEnum * const propertyType) const;


  /** getStringProperty returns the string property associated with
   *  propertyName in the output parameter propertyValue.  The caller
   *  should not modify or free propertyValue.  An error is returned if
   *  either parameter is NULL, propertyName is not a valid property,
   *  or if the property associated with propertyName is not a string
   *  property */
  iMQError getStringProperty(const char * const propertyName,
                             const char **      propertyValue) const;


  /*  setStringProperty sets the string property associated with
   *  propertyName to propertyValue.  The values for propertyName and
   *  propertyValue are copied, so the caller is free to modify these
   *  after getStringProperty returns.  An error is returned if either
   *  parameter is NULL, or if memory cannot be allocated to return the
   *  string. */
  iMQError setStringProperty(const char * const propertyName,
                             const char * const propertyValue);
  
  iMQError setBooleanProperty(const char     * const propertyName,
                              const PRBool           propertyValue);
  iMQError getBooleanProperty(const char     * const propertyName,
                                    PRBool   * const propertyValue) const;

  iMQError setByteProperty(const char     * const propertyName,
                           const PRInt8           propertyValue);
  iMQError getByteProperty(const char     * const propertyName,
                                 PRInt8   * const propertyValue) const;

  iMQError setShortProperty(const char     * const propertyName,
                            const PRInt16           propertyValue);
  iMQError getShortProperty(const char     * const propertyName,
                                 PRInt16   * const propertyValue) const;

  iMQError setIntegerProperty(const char     * const propertyName,
                              const PRInt32          propertyValue);
  iMQError getIntegerProperty(const char     * const propertyName,
                                    PRInt32  * const propertyValue) const;

  iMQError setLongProperty(const char     * const propertyName,
                           const PRInt64          propertyValue);
  iMQError getLongProperty(const char     * const propertyName,
                                 PRInt64  * const propertyValue) const;

  iMQError setFloatProperty(const char      * const propertyName,
                            const PRFloat32         propertyValue);
  iMQError getFloatProperty(const char      * const propertyName,
                                  PRFloat32 * const propertyValue) const;

  iMQError setDoubleProperty(const char      * const propertyName,
                             const PRFloat64         propertyValue);
  iMQError getDoubleProperty(const char      * const propertyName,
                                   PRFloat64 * const propertyValue) const;

  /** Deletes the propertyName property. */
  iMQError removeProperty(const char * propertyName);

  /** Necessary to implement HandledObject. */
  virtual HandledObjectType getObjectType() const;


  /** Add the contents of the file 'input' to the list of properties.
   *  The properties present before calling readFromFile are left *
   *  unchanged unless one of the properties in file collides with it.
   *
   *  The contents of 'input' must be in the following format:
   *  propertyName1 = propertyValue1
   *  propertyName2 = propertyValue2
   *  propertyName3 = propertyValue3
   *
   *  No whitespace may appear in a property name or property value.
   *
   *  @param fileName the name of the input file to read from
   *  @return IMQ_SUCCESS if successful and an error otherwise
   */
  iMQError readFromFile(const char * const fileName);

  // Methods from BasicTypeHashtable that this class wraps
  virtual iMQError  keyIterationStart();
  virtual PRBool    keyIterationHasNext();
  virtual iMQError  keyIterationGetNext(const char ** const key);
  virtual iMQError  print(FILE * const out);
  virtual iMQError  getNumKeys(PRUint32 * const numKeys) const;
  const char * toString( const char * const linePrefix = "" );

  /** Returns a clone of this object */
  Properties * clone() const;

  /** This is only here for backwards compatible reasons */
  BasicTypeHashtable * getHashtable();

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // This are not supported and are not implemented
  //
  Properties& operator=(const Properties& properties);
};


#endif // PROPERTIES_HPP
