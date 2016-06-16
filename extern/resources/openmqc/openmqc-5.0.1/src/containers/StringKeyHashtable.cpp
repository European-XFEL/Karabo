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
 * @(#)StringKeyHashtable.cpp	1.1 10/17/07
 */ 

#include "StringKeyHashtable.hpp"
#include "../basictypes/UTF8String.hpp"
#include "../util/UtilityMacros.h"
#include "../util/LogUtils.hpp"

/*
 *
 */
StringKeyHashtable::StringKeyHashtable()
{
  CHECK_OBJECT_VALIDITY();

  PRBool autoDeleteKey, autoDeleteValue;
  this->table = new BasicTypeHashtable(autoDeleteKey=PR_TRUE, 
                                       autoDeleteValue=PR_TRUE); 
}


/*
 *
 */
StringKeyHashtable::~StringKeyHashtable()
{
  CHECK_OBJECT_VALIDITY();

  DELETE( this->table );
}

/*
 *
 */
MQError
StringKeyHashtable::remove(const char *key)
{
  CHECK_OBJECT_VALIDITY();
  
  MQError errorCode = MQ_SUCCESS;

  RETURN_ERROR_IF_NULL(key);  
  RETURN_ERROR_IF(this->table == NULL, MQ_OUT_OF_MEMORY);  

  UTF8String keyu(key);

  monitor.enter();
  errorCode = this->table->removeEntry(&keyu);
  monitor.exit();

  if (errorCode != MQ_SUCCESS) {
    if (errorCode != MQ_NOT_FOUND)  {
    LOG_WARNING(( CODELOC, HASHTABLE_LOG_MASK, NULL_CONN_ID, errorCode,
        "Failed to remove key=%s from the StringKeyHashtable 0x%p because '%s' (%d)", 
         key, this, errorStr(errorCode), errorCode ));
    } else {
    LOG_FINE(( CODELOC, HASHTABLE_LOG_MASK, NULL_CONN_ID, errorCode,
        "Can't remove key=%s from the StringKeyHashtable 0x%p because '%s' (%d)", 
         key, this, errorStr(errorCode), errorCode ));
    }
  }

  return errorCode;
}

/*
 *
 */
MQError 
StringKeyHashtable::add(const char * key, Object * const value)
{
  CHECK_OBJECT_VALIDITY();

  MQError errorCode = MQ_SUCCESS;
  Object * prev = NULL;

  RETURN_ERROR_IF_NULL(key);
  RETURN_ERROR_IF_NULL(value);
  RETURN_ERROR_IF(this->table == NULL, MQ_OUT_OF_MEMORY);

  UTF8String * keyu = new UTF8String(key);

  RETURN_ERROR_IF(keyu == NULL, MQ_OUT_OF_MEMORY);

  monitor.enter();
  errorCode = this->table->getValueFromKey(keyu, (const Object** const)&prev);
  if (errorCode == MQ_SUCCESS) {
    LOG_INFO(( CODELOC, HASHTABLE_LOG_MASK, NULL_CONN_ID, errorCode,
        "key=%s already exists in StringKeyHashtable 0x%p, removing ...",
         key, this ));
    errorCode = remove(key);
    if (errorCode == MQ_SUCCESS) {
      errorCode = this->table->addEntry(keyu, value);
    }
  } else if (errorCode == MQ_NOT_FOUND) {
    errorCode = this->table->addEntry(keyu, value);
  }
  monitor.exit();

  if (errorCode != MQ_SUCCESS) {
    DELETE( keyu );
    LOG_SEVERE(( CODELOC, HASHTABLE_LOG_MASK, NULL_CONN_ID, errorCode, 
        "Failed to add key=%s to StringKeyHashtable 0x%p because '%s' (%d)",
         key, this, errorStr(errorCode), errorCode ));
  }

  return errorCode;
}


/*
 *
 */
MQError
StringKeyHashtable::get(const char * key, Object ** const value)
{
  CHECK_OBJECT_VALIDITY();

  MQError errorCode = MQ_SUCCESS;

  RETURN_ERROR_IF_NULL( key );
  RETURN_ERROR_IF_NULL( value );
  RETURN_ERROR_IF(this->table == NULL, MQ_OUT_OF_MEMORY);

  UTF8String keyu(key);


  monitor.enter();
  errorCode = this->table->getValueFromKey(&keyu, (const Object** const)value);
  monitor.exit();

  if (errorCode != MQ_SUCCESS) {
    if (errorCode == MQ_NOT_FOUND) {
      LOG_FINE(( CODELOC, HASHTABLE_LOG_MASK, NULL_CONN_ID, errorCode,
          "key=%s not found in StringKeyHashtable 0x%p", key, this ));
    } else {
      LOG_SEVERE(( CODELOC, HASHTABLE_LOG_MASK, NULL_CONN_ID, errorCode,
          "Failed to get key=%s from StringKeyHashtable 0x%p because '%s' (%d)",
           key, this, errorStr(errorCode), errorCode ));
    }
  }

  return errorCode;
}

