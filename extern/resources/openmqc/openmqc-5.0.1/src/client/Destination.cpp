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
 * @(#)Destination.cpp	1.4 06/26/07
 */ 

#include "Destination.hpp"
#include "../util/UtilityMacros.h"
#include "Connection.hpp"

static const UTF8String QUEUE_CLASS_NAME("com.sun.messaging.BasicQueue");
static const UTF8String TEMP_QUEUE_CLASS_NAME("com.sun.messaging.jmq.jmsclient.TemporaryQueueImpl");
static const UTF8String TOPIC_CLASS_NAME("com.sun.messaging.BasicTopic");
static const UTF8String TEMP_TOPIC_CLASS_NAME("com.sun.messaging.jmq.jmsclient.TemporaryTopicImpl");


/*
 *
 */
Destination::Destination(Connection * const connectionArg,
                         const UTF8String * const nameArg, 
                         const PRBool isQueueArg, 
                         const PRBool isTemporaryArg)
{
  CHECK_OBJECT_VALIDITY();

  this->connection     = connectionArg;
  this->isQueue     = isQueueArg;
  this->isTemporary = isTemporaryArg;

  if (nameArg != NULL) {
    this->name = (UTF8String*)nameArg->clone();
  } else {
    this->name = NULL;
  }
}

/*
 *
 */
Destination::Destination(const UTF8String * const nameArg, 
                         const UTF8String * const classNameArg,
                         Connection * const connectionArg)
{
  CHECK_OBJECT_VALIDITY();

  this->connection = connectionArg;

  // Set the name
  if (nameArg != NULL) {
    this->name = (UTF8String*)nameArg->clone();
  } else {
    this->name = NULL;
  }

  //
  // Set isQueue and isTemporary based on the class name
  //
  if (QUEUE_CLASS_NAME.equals(classNameArg)) { 
    this->isQueue     = PR_TRUE;
    this->isTemporary = PR_FALSE;
  } 
  else if (TEMP_QUEUE_CLASS_NAME.equals(classNameArg)) { 
    this->isQueue     = PR_TRUE;
    this->isTemporary = PR_TRUE;
  } 
  else if (TOPIC_CLASS_NAME.equals(classNameArg)) { 
    this->isQueue     = PR_FALSE;
    this->isTemporary = PR_FALSE;
  } 
  else if (TEMP_TOPIC_CLASS_NAME.equals(classNameArg)) { 
    this->isQueue     = PR_FALSE;
    this->isTemporary = PR_TRUE;
  } 
  // The class name was unrecognized, so delete the name so we don't
  // get in trouble.
  else {
    DELETE( this->name );
  }
}

/*
 *
 */
Destination::~Destination()
{
  CHECK_OBJECT_VALIDITY();

  DELETE(this->name);
  this->connection  = NULL;
  this->name        = NULL;
  this->isQueue     = PR_FALSE;
  this->isTemporary = PR_FALSE;
}


/*
 * This returns a deepcopy clone of this Destination.  The copy is not
 * associated with any session.
 */
Destination *
Destination::clone() const
{
  static const char FUNCNAME[] = "clone";
  CHECK_OBJECT_VALIDITY();
   
  Destination * dest = new Destination(this->getName(), this->getClassName(), this->connection);

  if (dest == NULL) {
    MQ_ERROR_TRACE(FUNCNAME, MQ_OUT_OF_MEMORY );
    return NULL;
  }

  // Make sure the initialization worked.  Cloning the name could have failed.
  if ((dest->getInitializationError() != IMQ_SUCCESS) ||
      ((this->getName() != NULL) && 
       !this->getName()->equals(dest->getName()))) 
  {
    
    if (dest->getInitializationError() != MQ_SUCCESS) {
      MQ_ERROR_TRACE(FUNCNAME, dest->getInitializationError() );
    } else {
      MQ_ERROR_TRACE( FUNCNAME, MQ_OUT_OF_MEMORY );
    }
    HANDLED_DELETE(dest);
    return NULL;
  }

  if ((this->getClassName() != NULL) && 
      !this->getClassName()->equals(dest->getClassName())) 
  {
    HANDLED_DELETE(dest);
  }

  return dest;
}


/**
 * @return the name of the destination
 */
const UTF8String * 
Destination::getName() const
{
  CHECK_OBJECT_VALIDITY();

  return this->name;
}

/*
 *
 */
PRBool
Destination::getIsQueue() const
{
  CHECK_OBJECT_VALIDITY();
  
  return this->isQueue;
}

/*
 *
 */
PRBool
Destination::getIsTemporary() const
{
  CHECK_OBJECT_VALIDITY();

  return this->isTemporary;
}

/*
 *
 */
iMQError
Destination::deleteDestination()
{
  CHECK_OBJECT_VALIDITY();

  // Only valid for temporary destinations
  if (!this->getIsTemporary()) {
    return IMQ_DESTINATION_NOT_TEMPORARY;
  }

  // Delete the destination at the broker
  ASSERT( this->connection != NULL );  
  if (this->connection != NULL) {
    RETURN_IF_ERROR( this->connection->deleteDestination(this) );    
  } else {
    return IMQ_BROKER_CONNECTION_CLOSED;
  }
  
  return IMQ_SUCCESS;
}


/*
 *
 */
const UTF8String * 
Destination::getClassName() const
{
  CHECK_OBJECT_VALIDITY();

 // BasicQueue
  if ((this->isQueue) && (!this->isTemporary)) {
    return &QUEUE_CLASS_NAME;
  }

  // TemporaryQueue
  if ((this->isQueue) && (this->isTemporary)) {
    return &TEMP_QUEUE_CLASS_NAME;
  }

  // BasicTopic
  if ((!this->isQueue) && (!this->isTemporary)) {
    return &TOPIC_CLASS_NAME;
  }

  // TemporaryTopic
  if ((!this->isQueue) && (this->isTemporary)) {
    return &TEMP_TOPIC_CLASS_NAME;
  }

  // Shouldn't ever get here
  ASSERT( PR_FALSE );
  return NULL;
}

/*
 *
 */
HandledObjectType
Destination::getObjectType() const 
{
  CHECK_OBJECT_VALIDITY();

  return DESTINATION_OBJECT;
}


/*
 *
 */
Connection *
Destination::getConnection() const
{
  CHECK_OBJECT_VALIDITY();

  return this->connection;
}
