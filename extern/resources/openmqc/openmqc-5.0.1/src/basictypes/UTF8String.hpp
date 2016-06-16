/*
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS HEADER.
 *
 * Copyright (c) 2000-2012 Oracle and/or its affiliates. All rights reserved.
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
 * @(#)UTF8String.hpp	1.4 06/26/07
 */ 

#ifndef UTF8STRING_HPP
#define UTF8STRING_HPP

#include "BasicType.hpp"
#include "../containers/ObjectVector.hpp"

/** Similar to a Java string.  The string is UTF8 encoded. */
// The size of the shortest string that must be encoded as a
// UTF8_LONG_STRING.
static const PRUint32 UTF8STRING_MIN_LONG_STRING_LENGTH = 0x00010000;
class UTF8String : public BasicType
{
private:

  PRUint8 * value;           // the UTF8 string
  PRUint32  bytesAllocated;  // charsInValue + 1 (assuming value != NULL)
  PRUint32  bytesInValue;    // the number of bytes in string
  PRBool    isLongString;    // true iff the length of value >= 2^16 (i.e. 
                             // it's length doesn't fit in an unsigned short)
  void init();
  void setString(const char * const value, const PRUint32 valueLength);

public:
  UTF8String();
  UTF8String(PRBool isLongString);
  UTF8String(const char * const value);
  UTF8String(const char * const value, const PRUint32 valueLength);
  ~UTF8String();

  /** Resets this string */
  void       reset();
 
  /** */
  iMQError setValue(const char * const value);

  // Currently, all this method does is return a pointer to value,
  // which we NULL terminated when we read it in.  Use it only for
  // debugging purposes.
  const PRUint8 *  toUCharStr() const;

  // getCharStr is basically just an alias for toUCharStr
  const char *  getCharStr() const;

  // calls getCharStr()
  const char * toString();

  const PRUint8 * getBytes() const;
  PRInt32 getBytesSize() const;

  /** Interprets this string as a unsigned 16-bit integer, and returns
   *  the result in uint16Value */
  iMQError getUint16Value(PRUint16 * const uint16Value);
  iMQError readLengthBytes(IMQDataInputStream * const in, PRBool checknull);

  // virtual methods from BasicType that must be implemented
  virtual TypeEnum      getType() const;
  virtual BasicType *   clone() const;
  virtual PRBool        equals(const BasicType * const object) const;
  virtual PLHashNumber  hashCode() const;
  virtual iMQError      read(IMQDataInputStream * const in);
  virtual iMQError      write(IMQDataOutputStream * const out) const;
  virtual iMQError      print(FILE * const file) const;

  

  // Returns the length of the string
  virtual PRInt32       length() const;

  // Returns a Vector of UTF8Strings that are the result of this
  // string being split on the delimeter string delim.  The strings do
  // not include the delimeter.
  iMQError tokenize(const char * const delimStr, ObjectVector ** const strVector ) const;

  virtual iMQError getBoolValue(PRBool * const valueArg) const;
  virtual iMQError getInt8Value(PRInt8 * const value) const;
  virtual iMQError getInt16Value(PRInt16 * const value) const;
  virtual iMQError getInt32Value(PRInt32 * const value) const;
  virtual iMQError getInt64Value(PRInt64 * const value) const;
  virtual iMQError getFloat32Value(PRFloat32 * const value) const;
  virtual iMQError getFloat64Value(PRFloat64 * const value) const;
  virtual iMQError getStringValue(const char ** const value) const;
  
//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  UTF8String(const UTF8String& string);
  UTF8String& operator=(const UTF8String& string);
};
 

#endif // UTF8STRING_HPP
