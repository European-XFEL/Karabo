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
 * @(#)BasicTypeHashtable.hpp	1.4 06/26/07
 */ 

#ifndef BASICTYPEHASHTABLE_HPP
#define BASICTYPEHASHTABLE_HPP

#include "../basictypes/Object.hpp"
#include "../basictypes/TypeEnum.hpp"
#include "../basictypes/BasicType.hpp"
#include "../error/ErrorCodes.h"
#include "../util/PRTypesUtils.h"
#include "Vector.hpp"
#include <stdio.h>
#include <plhash.h>
#include <prtypes.h>

// NSPR hashtable requires this
typedef int PRintn;
typedef unsigned int PRUintn;

/**
 * This class encapsulates a NSPR hashtable.  The keys to elements in
 * this hastable must be pointers to BasicType (or subclasses
 * thereof), and the values must be pointers to Object (or subclasses
 * thereof).
 */
static const PRUint32 HASHTABLE_MAX_STR_SIZE = 2000;
class BasicTypeHashtable : public Object {
private:
  /**
   * The NSPR C based hashtable that this class wraps.
   */
  PLHashTable *   plhash;

  /**
   * A struct for the operations that NSPR's hashtable uses to
   * allocate and free the table and entries.  
   */
  PLHashAllocOps  allocOps;

  /** The number of entries in the hash table */
  PRUint32        numEntries;

  /**
   * True iff the destructor should call delete on each key.
   */
  PRBool autoDeleteKeys;

  /**
   * True iff the destructor should call delete on each value. 
   */
  PRBool autoDeleteValues;

  /** Initialize the hash table */
  void init(const PRBool autoDeleteKeys, const PRBool autoDeleteValues);

  /** Vector that stores all of the keys.  This is used for iterating
   *  through the keys. */
  Vector keys;

  /** The current index into keys */
  PRUint32 iteratorIndex;

  /** stops the iterator */
  void keyIterationStop();


  /** This holds a string representation of the hashtable.  It is only
   * used for debugging purposes */
  char * hashtableStr;

public:
  /** Constructor.  Sets autoDeleteKeys = autoDeleteValues = true */
  BasicTypeHashtable();
  
  /** Constructor.  Sets autoDeleteKeys and autoDeleteValues to the
      parameter values */
  BasicTypeHashtable(const PRBool autoDeleteKeys, const PRBool autoDeleteValues);

  /** Sets this BasicTypeHashtable to a deepcopy of hashtable.  This
   *  only works if all of the values stored are BasicTypes.  It is
   *  only intended to be used by Properties::Properties(const
   *  Properties&) */
  BasicTypeHashtable(const BasicTypeHashtable& hashtable);

  /** Destructor */
  virtual ~BasicTypeHashtable();

  /** Deletes all members in the Hashtable, frees all memory
      associated with it, and reinitializes it. */
  virtual void reset();

  /** returns the number of keys stored in the Hashtable in the output
   *  parameter numKeys.  It returns an error if numKeys is NULL. */
  virtual iMQError  getNumKeys(PRUint32 * const numKeys) const;


  /** adds the key => value entry into the Hashtable.  It returns an
   *  error if key or value is NULL or if memory cannot be
   *  allocated. */
  virtual iMQError  addEntry(BasicType * const key, Object * const value);

  /** removeEntry removes the entry with the associated key from the
   *  table It returns an error if the key is invalid. */
  virtual iMQError  removeEntry(const BasicType * const key);

  /** getValueFromKey returns the value associated with key in the
   *  output parameter value.  It returns IMQ_NOT_FOUND if key is not
   *  located in the hashtable.  It returns an error if key or value is
   *  NULL. */
  virtual iMQError  getValueFromKey(const BasicType * const key, 
                                    const Object **   const value) const;

  /** This method prints the hashtable out to the file specified by
   *  'out'.  It invalidates the current iterator, if it is active. */
  virtual iMQError  print(FILE * const out);

  /** keyIterationStart, keyIterationHasNext, and keyIterationGetNext
   *  are used to iterate through the keys in the hashtable.  Use them 
   *  as follows (shown without error checking):
   *  
   *     hashtable->keyIterationStart();
   *     while(hashable->keyIterationHasNext()) {
   *       BasicType * key = NULL;
   *       hashtable->keyIterationgetNext(&key);
   *        // do your thing
   *     }
   *  
   *  
   *   Calls to addEntry, deleteEntry, or reset will invalidate the iteration. */
  virtual iMQError  keyIterationStart();
  virtual PRBool    keyIterationHasNext();
  virtual iMQError  keyIterationGetNext(const BasicType ** const key);

  /** Returns PR_TRUE iff the keys will be automatically deleted when 
   *  the hashtable is destructed */
  PRBool getAutoDeleteKeys() const;
  
  /** Returns PR_TRUE iff the values will be automatically deleted when 
   *  the hashtable is destructed */
  PRBool getAutoDeleteValues() const;

  /** The following six methods are only used for (de-)serializing the
   *  Hashtable.  The fields loadFactor, threshold, and capacity appear
   *  in the serialized form of a Java Hashtable, but they are not
   *  currently used by this class. */
  void      setLoadFactor(const PRFloat32 loadFactor);
  void      setThreshold(const PRInt32 threshold);
  void      setCapacity(const PRInt32 capacity);
  iMQError  getLoadFactor(PRFloat32 * const loadFactor) const;
  iMQError  getThreshold(PRInt32 * const threshold) const;
  iMQError  getCapacity(PRInt32 * const capacity) const;


  /** Returns a string representation of the hash table */
  const char * toString( const char * const linePrefix = "" );

  /**
   * Tests the BasicTypeHashtable class
   *
   * @param numIterations the number of times to perform the test
   * @param numKeyGroups the number of different keys of each type 
   * to put in the hashtable
   * @return IMQ_SUCCESS if the test succeeds and an error otherwise
   */
  static iMQError test(const PRInt32 numIterations, const PRInt32 numKeyGroups);

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // This are not supported and are not implemented
  //
  BasicTypeHashtable& operator=(const BasicTypeHashtable& hashtable);
};




#endif // BASICTYPEHASHTABLE_HPP
