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
 * @(#)SerialHandleManager.hpp	1.3 06/26/07
 */ 

#ifndef SERIALHANDLEMANAGER_HPP
#define SERIALHANDLEMANAGER_HPP

#include "../debug/DebugUtils.h"  // must be first in the file

#include <nspr.h>
#include <stdio.h>
#include "../basictypes/BasicType.hpp"
#include "../basictypes/TypeEnum.hpp"
#include "../error/ErrorCodes.h"
#include "../serial/Serialize.hpp"
#include "../containers/Vector.hpp"

// BASE_HANDLE is the first handle to allocate when (de-)serializing a 
// class.  After BASE_HANDLE the handles monotonically increase by 1.
static const PRUint32 SERIAL_HANDLE_MANAGER_BASE_HANDLE    = SERIALIZE_BASE_WIRE_HANDLE;
static const PRUint32 SERIAL_HANDLE_MANAGER_INVALID_HANDLE = 0xFFFFFFFF;

/**
 * Stores information about the handles used when (de-)serializing the
 * Java hashtable.
 *
 * Once a HandleInfo object has been successfully placed in the
 * SerialHandleManager, the SerialHandleManager owns it and will
 * delete it when the SerialHandleManager gets deleted.  */
class SerialHandleManager : public Object {
public:    
  /*
   * The SerialHandleManager stores both handles to class descriptions
   * and handles to objects.  The abstract base class Handle has two
   * implementing classes (ObjectHandle and ClassHandle) to reflect
   * this.  
   */
  enum HandleType {OBJECT_HANDLE, CLASS_HANDLE};

private:

  /* Abstract base class for ObjectHandleInfo and ClassHandleInfo */ 
  class HandleInfo {
  public:
    // we need this to be virtual so that the sub class destructors are called 
    virtual ~HandleInfo() {} 
    virtual HandleType  getType() const = 0;
    virtual PRBool      equals(const HandleInfo * const handleInfo) const = 0;
    virtual iMQError    print(FILE * const file) const = 0;
  };

  // Stores information about a handle to an object. 
  class ObjectHandleInfo : public HandleInfo {
  public:
    BasicType * objectValue;

    ObjectHandleInfo();
    ObjectHandleInfo(BasicType * const objectValue);
    ~ObjectHandleInfo();
    virtual HandleType  getType() const;
    virtual PRBool      equals(const HandleInfo * const handleInfo) const;
    virtual iMQError    print(FILE * const file) const;
  };

  // Stores information about a handle to a class description.
  class ClassHandleInfo : public HandleInfo {
  public:
    TypeEnum classType;

    ClassHandleInfo();
    ClassHandleInfo(const TypeEnum classType);
    ~ClassHandleInfo();
    virtual HandleType  getType() const;
    virtual PRBool      equals(const HandleInfo * const handleInfo) const;
    virtual iMQError    print(FILE * const file) const;
  };


  // A Vector of HandleInfo*, which stores information about each
  // Handle.
  Vector handles;

  // Initializes all member variables 
  void init();

  // setNextHandle sets the next handle to be allocated to handleInfo.
  // Returns an error if handleInfo is NULL or if memory cannot be
  // allocated.
  iMQError setNextHandle(HandleInfo * const handleInfo);

  // getInfoFromHandle returns the handle information associated
  // with handle in handleInfo.  An error is returned if handle is
  // invalid or if handleInfo is NULL.
  iMQError getInfoFromHandle(const PRUint32 handle,
                             HandleInfo ** const handleInfo) const;

  // getHandleFromInfo searches the vector of HandleInfo's for one
  // that matches handleInfo.  If a match is found then it is returned
  // in handle.  An error is returned if handleInfo or handle is NULL.
  // If no match is found, then handle will be set to 
  // SerialHandleManager::INVALID_HANDLE.
  iMQError getHandleFromInfo(const HandleInfo * const handleInfoToFind,
                             PRUint32 * const handle ) const;

public:
  SerialHandleManager();
  ~SerialHandleManager();
  void reset();

  // setNextHandleToObject sets the next handle to be allocated to
  // object.  object can be NULL.  It returns and error if memory
  // cannot be allocated.  If this call is successful, then
  // SerialHandleManager will free object when it is reset() or
  // destructed.  
  iMQError setNextHandleToObject(BasicType * object);

  // setNextHandleToClass sets the next handle to be allocated to
  // classType.  It returns and error if memory cannot be allocated.
  iMQError setNextHandleToClass(const TypeEnum classType);

  // getClassFromHandle returns the class type associated with handle.
  // If classType is NULL, handle is not a valid handle, or if handle
  // is not a handle to a class, then an error is returned.
  iMQError getClassFromHandle(const PRUint32 handle,
                              TypeEnum * const classType) const;

  // getObjectCloneFromHandle returns a clone of the object associated
  // with handle.  If object is NULL, handle is not a valid handle, or
  // if handle is not a handle to an object, then an error is
  // returned.
  iMQError getObjectCloneFromHandle(const PRUint32 handle,
                                    BasicType ** const object) const;

  // getHandleFromBasicType searches the vector of HandleInfo's for a
  // SerialHandleManager::ObjectHandleInfo object that matches object.
  // If a match is found then it is returned in handle.  An error is
  // returned if object or handle is NULL.  If no match is found, then
  // handle will be set to SerialHandleManager::INVALID_HANDLE.  This
  // method sets up a SerialHandleManager::ObjectHandleInfo object and
  // then calls getHandleFromInfo.
  iMQError getHandleFromBasicType( const BasicType * const object,
                                   PRUint32 * handle );

  // getHandleFromClass searches the vector of HandleInfo's for a
  // SerialHandleManager::ClassHandleInfo object matches classType.
  // If a match is found then it is returned in handle.  If no match
  // is found, then handle will be set to SerialHandleManager::
  // INVALID_HANDLE.  This method sets up a SerialHandleManager::
  // ObjectHandleInfo object and then calls getHandleFromInfo.  An
  // error is returned if handle is NULL.
  iMQError getHandleFromClass( const TypeEnum classType,
                               PRUint32 * handle );
  
  // Print the vector of handles out to file.  This is currently not
  // implemented.
  iMQError print(FILE * const file) const;


//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  SerialHandleManager(const SerialHandleManager& serialHandleManager);
  SerialHandleManager& operator=(const SerialHandleManager& serialHandleManager);
};


#endif // SERIALHANDLEMANAGER_HPP


