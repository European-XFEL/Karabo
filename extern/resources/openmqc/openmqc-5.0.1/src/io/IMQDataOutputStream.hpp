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
 * @(#)IMQDataOutputStream.hpp	1.3 06/26/07
 */ 

#ifndef IMQDATAOUTPUTSTREAM_HPP
#define IMQDATAOUTPUTSTREAM_HPP

#include <prtypes.h>
#include "../error/ErrorCodes.h"
#include "../util/PRTypesUtils.h"
#include "../basictypes/Object.hpp"

/** 
 * This class is the the abstract base class that defines how to write to a
 * stream.
 */
class IMQDataOutputStream : public Object {
public:
// These are the only methods non-abstract subclasses must implement
// WriteUint[16|32|64] depend on the endianness of the stream so
// IMQDataOutputStream cannot implement them by calling writeUint8.

  /** 
   * This method writes the 8-bit unsigned integer value to the output stream.
   *
   * @param value is number to write
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError writeUint8(const PRUint8 value) = 0;

  /** 
   * This method writes the 16-bit unsigned integer value to the output stream.
   *
   * @param value is number to write
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError writeUint16(const PRUint16 value) = 0;

  /** 
   * This method writes the 32-bit unsigned integer value to the output stream.
   *
   * @param value is number to write
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError writeUint32(const PRUint32 value) = 0;

  /** 
   * This method writes the 64-bit unsigned integer value to the output stream.
   *
   * @param value is number to write
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError writeUint64(const PRUint64 value) = 0;

// IMQDataOutputStream provides default implementations of these
// by calling the unsigned counterparts and casting the result.

  /** 
   * This method writes the boolean value to the output stream as an 8-bit
   * boolean.
   *
   * @param value is boolean to write
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError writeBoolean(const PRBool value);

  /** 
   * This method writes the 8-bit signed integer value to the output stream.
   *
   * @param value is number to write
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError writeInt8(const PRInt8 value);

  /** 
   * This method writes numToWrite 8-bit unsigned integers from the value array
   * to the output stream.
   *
   * @param value is array to write
   * @param numToWrite is the number of 8-bit unsigned integers to write
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError writeUint8Array(const PRUint8 values[], 
                                   const PRUint32 numToWrite);

  /** 
   * This method writes the 16-bit signed integer value to the output stream.
   *
   * @param value is number to write
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError writeInt16(const PRInt16 value);

  /** 
   * This method writes the 32-bit signed integer value to the output stream.
   *
   * @param value is number to write
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError writeInt32(const PRInt32 value);

  /** 
   * This method writes the 64-bit signed integer value to the output stream.
   *
   * @param value is number to write
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError writeInt64(const PRInt64 value);

  /** 
   * This method writes the 32-bit floating point number value to the output
   * stream.
   *
   * @param value is number to write
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError writeFloat32(const PRFloat32 value);

  /** 
   * This method writes the 64-bit floating point number value to the output
   * stream.
   *
   * @param value is number to write
   * @returns IMQ_SUCCESS if the read was successful and false otherwise.  
   */
  virtual iMQError writeFloat64(const PRFloat64 value);
};

#endif // IMQDATAINPUTSTREAM_HPP
