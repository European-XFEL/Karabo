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
 * @(#)BasicType.hpp	1.4 06/26/07
 */ 

#ifndef BASICTYPE_HPP
#define BASICTYPE_HPP

#include "TypeEnum.hpp"
#include "Object.hpp"
#include "../io/IMQDataInputStream.hpp"
#include "../io/IMQDataOutputStream.hpp"
#include "../error/ErrorCodes.h"

#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <plhash.h>


/**
 * Basictype is the abstract type for all basic types (e.g. Boolean,
 * Byte, Integer, ...).  */
class BasicType : public Object {
public:
  /** @return true */
  virtual PRBool getIsBasicType() const;

  /** 
   * @return the TypeEnum corresponding of the implementing class
   * (e.g. Integer returns INTEGER_TYPE).
   */
  virtual TypeEnum     getType() const = 0;

  /** 
   * Creates a deep copy of the object and returns it.  If a deep copy
   * cannot be performed (e.g. out of memory), then NULL is returned.
   * The caller is responsible for freeing the object.
   *
   * @return a deep copy of the object
   */
  virtual BasicType *  clone() const = 0;

  /**
   * Returns PR_TRUE iff object is equal to this object.
   *
   * @param object the object to test for equality
   * @return PR_TRUE iff object is the same type as 'this' and they
   *   have the same value.
   */
  virtual PRBool       equals(const BasicType * const object) const = 0;
  
  /**
   * Returns a hash code for this object.
   */
  virtual PLHashNumber hashCode() const = 0;

  /**
   * Initializes the object from the binary value read from the stream in. 
   *
   * @param in the stream to read from
   */
  virtual iMQError     read(IMQDataInputStream * const in) = 0;

  /**
   * Writes the object's value out in binary form to out.
   *
   * @param out the stream to write to
   */
  virtual iMQError     write(IMQDataOutputStream * const out) const = 0;

  /** 
   * Print the object to file in text form.
   *
   * @param file to print the object to.
   */
  virtual iMQError     print(FILE * const file) const = 0;


  // These conversion routines are used by the Properties class The
  // BasicType class provides default definitions for these converion
  // routine which simply an IMQ_INVALID_TYPE_CONVERSION error.
  virtual iMQError     getBoolValue(PRBool * const value) const;
  virtual iMQError     getInt8Value(PRInt8 * const value) const;
  virtual iMQError     getInt16Value(PRInt16 * const value) const;
  virtual iMQError     getInt32Value(PRInt32 * const value) const;
  virtual iMQError     getInt64Value(PRInt64 * const value) const;
  virtual iMQError     getFloat32Value(PRFloat32 * const value) const;
  virtual iMQError     getFloat64Value(PRFloat64 * const value) const;
  virtual iMQError     getStringValue(const char ** const value) const;

  /**
   * @return a char* representation of the basic type.
   */
  virtual const char *       toString() = 0;


};


#endif // BASICTYPE_HPP




