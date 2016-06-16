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
 * @(#)IMQDataInputStream.hpp	1.3 06/26/07
 */ 

#ifndef IMQDATAINPUTSTREAM_HPP
#define IMQDATAINPUTSTREAM_HPP

#include <nspr.h>

#include "../error/ErrorCodes.h"
#include "../util/PRTypesUtils.h"
#include "../basictypes/Object.hpp"

/** 
 * This class is the the abstract base class that defines how to read
 * from a stream.  */
class IMQDataInputStream : public Object {
public:
// The following five methods are the only methods non-abstract subclasses
// must implement.  ReadUint[16|32|64] depend on the endianness of the stream
// so IMQDataInputStream cannot implement them by calling readUint8.

  /** Return true iff at the end of the stream. */ 
  virtual PRBool    endOfStream() const = 0;

  /** 
   * This method reads an 8-bit unsigned integer from the input stream and
   * returns the result in value.
   *
   * @param value is the output parameter where the result is placed.
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.
   */
  virtual iMQError  readUint8(PRUint8 * const value) = 0;

  /** 
   * This method reads a 16-bit unsigned integer from the input stream and
   * returns the result in value.
   *
   * @param value is the output parameter where the result is placed.
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.
   */
  virtual iMQError  readUint16(PRUint16 * const value) = 0;

  /** 
   * This method reads a 32-bit unsigned integer from the input stream and
   * returns the result in value.
   *
   * @param value is the output parameter where the result is placed.
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.
   */
  virtual iMQError  readUint32(PRUint32 * const value) = 0;

  /** 
   * This method reads a 64-bit unsigned integer from the input stream and
   * returns the result in value.
   *
   * @param value is the output parameter where the result is placed.
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.
   */
  virtual iMQError  readUint64(PRUint64 * const value) = 0;

// IMQDataInputStream provides default implementations of these eight methods
// by calling the unsigned counterparts and casting the result.

  /** 
   * This method reads an 8-bit boolean from the input stream and returns the
   * result in value.
   *
   * @param value is the output parameter where the result is placed.
   * @returns IMQ_SUCCESS if the read was successful and false otherwise. 
   */
  virtual iMQError  readBoolean(PRBool * const value);

  /** 
   * This method reads an array of 8-bit unsigned integers from the input stream
   * and returns the result in value.  The value array must have at least
   * numToRead elements.
   *
   * @param value is the output parameter where the result is placed.
   * @param numToRead is the number of 8-bit unsigned integers to read
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError  readUint8Array(PRUint8 * const values, 
                                   const PRUint32 numToRead);

  /** 
   * This method reads an 8-bit signed integer from the input stream and returns
   * the result in value.
   *
   * @param value is the output parameter where the result is placed.
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError  readInt8(PRInt8 * const value);

  /** 
   * This method reads a 16-bit signed integer from the input stream and returns
   * the result in value.
   *
   * @param value is the output parameter where the result is placed.
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError  readInt16(PRInt16 * const value);

  /** 
   * This method reads a 32-bit signed integer from the input stream and returns
   * the result in value.
   *
   * @param value is the output parameter where the result is placed.
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError  readInt32(PRInt32 * const value);

  /** 
   * This method reads a 64-bit signed integer from the input stream and returns
   * the result in value.
   *
   * @param value is the output parameter where the result is placed.
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError  readInt64(PRInt64 * const value);

  /** 
   * This method reads a 32-bit floating point number from the input stream and
   * returns the result in value.
   *
   * @param value is the output parameter where the result is placed.
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError  readFloat32(PRFloat32 * const value);

  /** 
   * This method reads a 64-bit floating point number from the input stream and
   * returns the result in value.
   *
   * @param value is the output parameter where the result is placed.
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError  readFloat64(PRFloat64 * const value);
};


#endif // IMQDATAINPUTSTREAM_HPP
