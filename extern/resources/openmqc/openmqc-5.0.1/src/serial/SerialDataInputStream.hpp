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
 * @(#)SerialDataInputStream.hpp	1.3 06/26/07
 */ 

#ifndef SERIALINPUTSTREAM_H
#define SERIALINPUTSTREAM_H

#include "../io/IMQDataInputStream.hpp"
#include "../util/PRTypesUtils.h"
#include "../basictypes/TypeEnum.hpp"
#include "../basictypes/BasicType.hpp"
#include "../error/ErrorCodes.h"
//#include "SerialHashtable.hpp"
#include "../containers/BasicTypeHashtable.hpp"
#include "SerialHandleManager.hpp"

#include <nspr.h>


/** 
 * SerialDataInputStream is used to read values from a Transport stream.  All
 * data stored in the stream are assumed to be in network byte order.  It is
 * implemented internally by storing a byte array of the entire stream's
 * contents.  One of its primary methods is readHashTable, which de-serializes a
 * Java Hashtable stored in the stream.
 */
class SerialDataInputStream : public IMQDataInputStream {
private:
  /** true iff the stream has been initialized */
  PRBool     isValid;        
  
  /** dynamically allocated array that stores the stream */
  PRUint8 *  streamBytes;    
  
  /** the length of streamBytes */
  PRUint32   streamLength;   

  /** the current location in the input stream */
  PRUint32   streamLoc;      

  /** keeps track of the handle references that appear in the stream */
  SerialHandleManager handleManager;

  /** initializes all of the member variables */
  void init();

  //
  // These are the functions that deal primarily with deserializing
  // a Java Hashtable
  //

  /**
   * This method reads in all of the data associated with the serialized Java
   * Hashtable.  This includes reading in the loadFactor, threshold, capacity,
   * number of entries, and each <key,value> pair.  This method constructs a new
   * hash table from the data that it reads, and returns this
   * hash table in the output parameter hashtable.  If this method is
   * successful, the caller is responsible for freeing hashtable.  This method
   * does not read in the class description of the Hashtable.  An error is
   * returned if hashtable is NULL or if the stream location is not the
   * beginning of a valid serialized Java Hashtable.
   *
   * @param hashtable is output parameter where the deserialized hashtable is
   *  placed
   * @returns IMQ_SUCCESS if successful and an error otherwise
   */
  iMQError readHashtableData(BasicTypeHashtable * const hashtable);

  /** 
   * This method reads the class description from the input stream, and returns
   * the type of the class in the output parameter classType.  The class
   * description can be either a description of a new class, a reference to a
   * previous class description, or the NULL class description (used for classes
   * without superclasses).  An error is returned if classType is NULL or if the
   * current stream location is not the beginning of a valid Java serialized
   * class description.
   *
   * @param classType is output parameter where the deserialized classType is
   *  placed
   * @returns IMQ_SUCCESS if successful and an error otherwise */
  iMQError readClassDesc(TypeEnum * const classType);

  /** 
   * This method reads the class description from the input stream, and returns
   * the type of the class in the output parameter classType.  The class
   * description must be a description of a new class.  An error is returned if
   * classType is NULL or if the current stream location is not the beginning of
   * a new valid Java serialized class description.
   *
   * @param classType is output parameter where the deserialized classType is
   *  placed
   * @returns IMQ_SUCCESS if successful and an error otherwise 
   */
  iMQError readNewClassDesc(TypeEnum * const classType);


  /** 
   * This method reads the class description reference from the input stream,
   * looks up the reference, and returns the type of the class in the output
   * parameter classType.  An error is returned if classType is NULL or if the
   * current stream location is not the beginning of a valid reference to a
   * previously occuring Java class.  
   *
   * @param classType is output parameter where the deserialized classType is
   *  placed
   * @returns IMQ_SUCCESS if successful and an error otherwise    
   */
  iMQError readReferenceClassDesc(TypeEnum * const classType);

  /**
   * This method reads in a basic object (Boolean, Integer, Float, etc.) from
   * the input stream, and returns a new object in the output parameter
   * valueObject.  It reads the class description from the input stream, creates
   * an object of that type, and then initializes that object with the data that
   * appears in the input stream.  It returns an error if valueObject is NULL or
   * if the input stream location is not at the beginning of a valid basic type.
   * If the call is successful, the caller is responsible for freeing
   * valueObject.  
   * 
   * @param object is the output parameter where a pointer to the
   *  deserialized object is placed 
   * @returns IMQ_SUCCESS if successful and an error otherwise 
   */
  iMQError readSerializedBasicObject(BasicType ** const object);


  /** 
   * This method reads the object reference from the input stream, looks up the
   * reference, and returns a clone of the object.  An error is returned if
   * object is NULL or if the current stream location is not the beginning of a
   * valid reference to a previously occuring Java object.  If the call is
   * successful, the caller is responsible for freeing object.  
   *
   * @param object is the output parameter where a pointer to the
   *  deserialized object is placed 
   * @returns IMQ_SUCCESS if successful and an error otherwise 
   */
  iMQError readReferenceObject(BasicType ** const object);


  /**
   * This method reads numExpectedBytes from the input stream one at a time, and
   * compares each one to the corresponding byte of expectedBytes[].  It returns
   * an error if there is a mismatch between expectedBytes[] and the bytes that
   * were actually read.  
   *
   * @param expectedBytes is the array of expected bytes
   * @param numExpectedBytes is the number of expected bytes to read from the 
   *  stream.
   * @returns IMQ_SUCCESS if successful and an error otherwise 
   */
  iMQError consumeExpectedBytes(const PRUint8 expectedBytes[], 
                                const PRUint32 numExpectedBytes);

public:
  SerialDataInputStream();
  ~SerialDataInputStream();

  /**
   * This method frees all memory associated with this stream, and reinitialize
   * it.  
   */
  void reset();


  /** 
   * This method sets the input stream to stream[] by copying the contents of
   * stream[] into an internal buffer.  The parameter stream[] has size
   * streamSize.  It returns an error if stream[] is NULL or if memory cannot be
   * allocated.
   *
   * @param stream is the array of bytes to copy into this object
   * @param streamSize is the size of the stream to copy into this object
   * @returns IMQ_SUCCESS if successful and an error otherwise 
   */
  iMQError setNetOrderStream(const PRUint8 stream[], const PRUint32 streamSize);

  /** 
   * This method reads a serialized hash table from the stream into
   * the hashtable parameter.  An error is returned if hashtable is
   * NULL or if the current location in the stream is not a valid
   * serialized Java Hashtable.
   *
   * @param hashtable is output parameter where the deserialized hashtable is
   *  placed
   * @returns IMQ_SUCCESS if successful and an error otherwise 
   */
  iMQError readHashtable(BasicTypeHashtable * const hashtable);


  //
  // These are the virtual functions from the abstract base class
  // IMQDataInputStream that this class must implement
  //
  virtual PRBool    endOfStream() const;
  virtual iMQError  readUint8(PRUint8 * const value);
  virtual iMQError  readUint16(PRUint16 * const value);
  virtual iMQError  readUint32(PRUint32 * const value);
  virtual iMQError  readUint64(PRUint64 * const value);



  // Tests reading in corrupted hashtables
  static iMQError test(const char * inputTestFile, const PRBool doExhaustiveCorruptionTest);


//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  SerialDataInputStream(const SerialDataInputStream& in);
  SerialDataInputStream& operator=(const SerialDataInputStream& in);
};

#endif // SERIALINPUTSTREAM_H





