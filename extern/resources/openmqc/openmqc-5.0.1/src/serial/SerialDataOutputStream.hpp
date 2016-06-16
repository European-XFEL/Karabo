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
 * @(#)SerialDataOutputStream.hpp	1.3 06/26/07
 */ 

#ifndef SERIALDATAOUTPUTSTREAM_H
#define SERIALDATAOUTPUTSTREAM_H

#include "../io/IMQDataOutputStream.hpp"
#include "../util/PRTypesUtils.h"
#include "../basictypes/TypeEnum.hpp"
#include "../basictypes/BasicType.hpp"
#include "../error/ErrorCodes.h"
#include "../containers/BasicTypeHashtable.hpp"
#include "SerialHandleManager.hpp"

// INITIAL_STREAM_SIZE is the initial size of the output stream.
// Make this value smaller to conserve memory, and make it larger to
// eliminate the need to copy the buffer into a bigger buffer
// when it runs out of space.  This should be set based on the size
// of average iMQ packets.
static const PRInt32 SERIAL_DATA_OUTPUT_STREAM_INITIAL_STREAM_SIZE = 2048;
/** 
 * This class is used to write values to a byte array stream.  All
 * data written to the stream are written in network byte order.  It
 * is implemented internally by storing a byte array of the entire
 * stream's contents.  One of its primary methods is writeHashTable,
 * which serializes a Java Hashtable stored in the stream.  */
class SerialDataOutputStream : public IMQDataOutputStream {
private:

  /** dynamically allocated array that stores the stream */
  PRUint8 *  streamBytes;
  
  /** the length of streamBytes */
  PRInt32    streamLength;   

  /** the current location in the input stream */
  PRInt32    streamLoc;     

  /** keeps track of the handle references that appear in the stream */
  SerialHandleManager handleManager;

  /** init() initializes all of the member variables */
  void init();

  /**
   * Writes the data associated with hashtable to the stream.  This
   * includes writing the loadFactor, the threshold, the capacity, the
   * number of elements, and each <key,value> pair.  An error is
   * returned if hashtable is NULL or if there is some other error
   * writing to the stream. 
   * @param hashtable the hashtable to write
   * @return IMQ_SUCCESS if successful and an error otherwise
   */
  iMQError writeHashtableData(BasicTypeHashtable * const hashtable);
  
  /**
   * Writes a basic object (Boolean, Integer, Float, etc.) to the
   * output stream.  It first determines if the object is a new object
   * or a reference to a previous object.  If it is a reference, then
   * it calls writeReference.  Otherwise, it calls
   * writeSerializedNewBasicObject. 
   * @param value the basic type object to write to the file
   * @return IMQ_SUCCESS if successful and an error otherwise
   */
  iMQError writeSerializedBasicObject(const BasicType * const value);

  /**
   * Writes a basic object (Boolean, Integer, Float, etc.) to the
   * output stream.  It first either writes the class description or a
   * reference to the class description to the output stream.
   * @param value the basic type object to write to the file
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError writeSerializedNewBasicObject(const BasicType * const value);

  /**
   * Writes the serialized class description for classType.  If
   * classType has already been serialized then it writes a reference
   * to the original class description.  It also recursively writes
   * the super class descriptions.  Valid values for classType are
   * BOOLEAN_TYPE, BYTE_TYPE, SHORT_TYPE, INTEGER_TYPE, LONG_TYPE,
   * FLOAT_TYPE, DOUBLE_TYPE, NUMBER_TYPE, and NULL_TYPE. 
   * @param classType the type of the class to write the description for 
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError writeClassDesc(const TypeEnum classType);

  /**
   * Writes the handle reference to the output stream.  handle can
   * either be a handle to a previously serialized object or a
   * previous class description.  It also writes the TC_REFERENCE byte
   * to the stream.  It returns an error if the stream cannot be
   * written to. 
   * @param handle the handle of the reference to write to the stream
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError writeReference(const PRUint32 handle);

  /** a synonym for writeUint8 */
  iMQError writeByte(const PRUint8 byteToWrite);

  /** a synonym for writeUint8Array */
  iMQError writeBytes(const PRUint8 * const bytesToWrite,
                      const PRInt32 bytesToWriteLength);


  /**
   * Increases the size of the output buffer, streamBytes, if it is
   * full by calling increaseBuffer().  An error is returned if the
   * buffer is full and memory cannot be allocated. */
  iMQError increaseBufferIfNeeded();

  /**
   * increases the size of the output buffer, streamBytes, by setting
   * it to the max of INITIAL_STREAM_SIZE and twice its current size.
   * An error is returned if memory cannot be allocated. */
  iMQError increaseBuffer();

public:
  SerialDataOutputStream();
  ~SerialDataOutputStream();
  
  /** frees all memory associated with this stream, and reinitialize
      it. */
  void reset();

  /**
   * writes hashtable to the stream as a Java serialized hashtable.
   * An error is returned if there is some other error writing to the
   * stream. */
  iMQError writeHashtable(BasicTypeHashtable * const hashtable);

// These are the methods of IMQDataOutputStream that must be
// implemented
  virtual iMQError writeUint8(const PRUint8 value);
  virtual iMQError writeUint16(const PRUint16 value);
  virtual iMQError writeUint32(const PRUint32 value);
  virtual iMQError writeUint64(const PRUint64 value);

  /** @return the number of bytes that have been written to the output
      stream */
  PRInt32 numBytesWritten() const;

  /**
   * Returns a pointer to the array of bytes.  Efficiency is
   * emphasized instead of safety.  The caller should not alter what
   * is returned.
   *
   * @return a pointer to the array of bytes that represents this stream.
   * @see numBytesWritten 
   */
  const PRUint8 * getStreamBytes() const;


  /**
   * This method returns the bytes that were written out to the
   * stream, and resets the stream.  It is primarily used when an
   * object (such as a UTF8String) must be serialized and the
   * resulting buffer used without making copy.
   *
   * @param bytes the bytes written out to the stream.  The caller is responsible
   *  for freeing this buffer.
   * @param bytesSize the number of bytes written out to the stream.
   * @return IMQ_SUCCESS if successful and an error otherwise */
  iMQError exportStreamBytes(PRUint8 ** const bytes, PRInt32 * const bytesSize);
  

  /** this is only used for debugging purposes */
  iMQError  writeToFile(FILE * out);

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  SerialDataOutputStream(const SerialDataOutputStream& out);
  SerialDataOutputStream& operator=(const SerialDataOutputStream& out);
};




#endif // SERIALDATAOUTPUTSTREAM_H
