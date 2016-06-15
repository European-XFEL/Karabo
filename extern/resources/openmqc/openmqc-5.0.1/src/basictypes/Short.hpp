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
 * @(#)Short.hpp	1.5 06/26/07
 */ 

#ifndef SHORT_HPP
#define SHORT_HPP

#include "BasicType.hpp"
#include <prtypes.h>

/** Similar to Java's Short class */
static const PRInt16 SHORT_MIN_VALUE     = SHRT_MIN;
static const PRInt16 SHORT_MAX_VALUE     = SHRT_MAX;
static const PRInt16 SHORT_DEFAULT_VALUE = 0;
static const PRInt32 SHORT_MAX_STR_SIZE  = 7; // "-32768\0"
class Short : public BasicType 
{
protected:

  PRInt16 value;
  char * valueStr;

public:
  Short();
  Short(const PRInt16 value);
  virtual ~Short();
  void     setValue(const PRInt16 value);
  PRInt16  getValue() const;

  //
  // virtual functions to implement from BasicType
  //
  virtual TypeEnum      getType() const;
  virtual PRBool        equals(const BasicType * const object) const;
  virtual PLHashNumber  hashCode() const;
  virtual iMQError      read(IMQDataInputStream * const in);
  virtual iMQError      write(IMQDataOutputStream * const out) const;
  virtual iMQError      print(FILE * const file) const;
  virtual BasicType *   clone() const;
  virtual const char *  toString();

  virtual iMQError     getInt16Value(PRInt16 * const value) const;
  virtual iMQError     getInt32Value(PRInt32 * const value) const;
  virtual iMQError     getInt64Value(PRInt64 * const value) const;
  virtual iMQError     getStringValue(const char ** const value) const;

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  Short(const Short& shortValue);
  Short& operator=(const Short& shortValue);
};


#endif
