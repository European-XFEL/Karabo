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
 * @(#)iMQConnectionShim.cpp	1.30 11/09/07
 */ 

#include "mqconnection-priv.h"
#include "shimUtils.hpp"
#include "../client/Connection.hpp"
#include "xaswitch.hpp"

static MQError mqAckModeToAckMode(MQAckMode mqmode, AckMode * mode);

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQCreateConnection(MQPropertiesHandle propertiesHandle,
                     ConstMQString username,
                     ConstMQString password,
                     ConstMQString clientID,
                     MQConnectionExceptionListenerFunc exceptionListener,
                     void *                            exceptionCallBackData,
                     MQConnectionHandle *    connectionHandle)

{
  return MQCreateConnectionExt(propertiesHandle, username, password, clientID,
                                 exceptionListener, exceptionCallBackData,
                                 NULL, NULL, PR_FALSE, connectionHandle);
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQCreateXAConnection(MQPropertiesHandle propertiesHandle,
                     ConstMQString username,
                     ConstMQString password,
                     ConstMQString clientID,
                     MQConnectionExceptionListenerFunc exceptionListener,
                     void *                            exceptionCallBackData,
                     MQConnectionHandle *    connectionHandle)

{
  return MQCreateConnectionExt(propertiesHandle, username, password, clientID,
                                 exceptionListener, exceptionCallBackData,
                                 NULL, NULL, PR_TRUE, connectionHandle);
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQCreateConnectionExt(MQPropertiesHandle propertiesHandle,
                        ConstMQString  username,
                        ConstMQString  password,
                        ConstMQString  clientID,
                        MQConnectionExceptionListenerFunc exceptionListener,
                        void *                            exceptionCallBackData,
                        MQCreateThreadFunc createThreadFunc,
                        void*              createThreadFuncData,
                        MQBool             isXA,
                        MQConnectionHandle *    connectionHandle)
{
  static const char FUNCNAME[] = "MQCreateConnectionExt";
  MQError errorCode = MQ_SUCCESS;
  Properties * properties = NULL;
  Connection * connection = NULL;
  UTF8String * usernameStr = NULL;
  UTF8String * passwordStr = NULL;
  UTF8String * clientIDStr = NULL;
  bool propertiesReleased = false;
  
  CLEAR_ERROR_TRACE(PR_FALSE);
  //
  // Validate the parameters
  //
  properties = (Properties*)getHandledObject(propertiesHandle.handle, 
                                             PROPERTIES_OBJECT);
  CNDCHK( properties == NULL, MQ_STATUS_INVALID_HANDLE );

  if ((connectionHandle == NULL) || (username == NULL) || (password == NULL)) {
    ERRCHK( MQ_NULL_PTR_ARG );
  }

  // Initialize connectionHandle
  connectionHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Create a connection object
  MEMCHK( connection = new Connection() );
  ERRCHK( connection->getInitializationError() );
  ERRCHK( Connection::versionCheck(PR_FALSE) );

  // Convert username and password to UTF8Strings
  usernameStr = new UTF8String(username);
  passwordStr = new UTF8String(password);
  if( (usernameStr == NULL) || 
      (passwordStr == NULL) ||
      (STRCMP(usernameStr->getCharStr(), username) != 0) ||
      (STRCMP(passwordStr->getCharStr(), password) != 0))
  {
    DELETE( usernameStr );
    DELETE( passwordStr );
    ERRCHK( MQ_OUT_OF_MEMORY );
  }

  if (clientID != NULL) {
    clientIDStr = new UTF8String(clientID);
    if (clientIDStr == NULL || (STRCMP(clientIDStr->getCharStr(), clientID) != 0) ) {
      DELETE( clientIDStr );
      ERRCHK( MQ_OUT_OF_MEMORY );
    }
  }

  // Whether the openConnection succeeds or fails, properties is
  // now invalid outside of the library.
  properties->setIsExported(PR_FALSE);
  releaseHandledObject(properties);
  propertiesReleased = true;

  // Open a connection based on properties, username, and password
  ERRCHK( connection->openConnection(properties, usernameStr, passwordStr, 
                                     clientIDStr,
                                     exceptionListener, exceptionCallBackData, 
                                     createThreadFunc, createThreadFuncData) );
  if (isXA == MQ_TRUE) {
      connection->setIsXA();
  }
  // This connection handle is valid outside of the library
  connection->setIsExported(PR_TRUE);

  // Set the output handle
  connectionHandle->handle = connection->getHandle();

  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  if (!propertiesReleased) {
    releaseHandledObject(properties);
    freeHandledObject(propertiesHandle.handle, PROPERTIES_OBJECT);
  }
  HANDLED_DELETE( connection );
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}


EXPORTED_SYMBOL MQStatus 
MQGetXAConnection(MQConnectionHandle *connectionHandle)
{
  static const char FUNCNAME[] = "MQGetXAConnection";
  MQError errorCode = MQ_SUCCESS;
  MQConnectionHandle invalidHandle = MQ_INVALID_HANDLE; 

  CLEAR_ERROR_TRACE(PR_FALSE); 

  if ((connectionHandle == NULL)) { 
    RETURN_STATUS( MQ_NULL_PTR_ARG );
  }

  *connectionHandle = mq_getXAConnection(); 
  CNDCHK( (*connectionHandle).handle == invalidHandle.handle, MQ_STATUS_INVALID_HANDLE );
  RETURN_STATUS( MQ_SUCCESS ); 

Cleanup:
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQCloseConnection(const MQConnectionHandle connectionHandle)
{
  static const char FUNCNAME[] = "MQCloseConnection";
  MQError errorCode = MQ_SUCCESS;
  Connection * connection = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);
                                                          
  // Convert connectionHandle to a Connection pointer
  connection = (Connection*)getHandledObject(connectionHandle.handle, 
                                             CONNECTION_OBJECT);
  CNDCHK( connection == NULL, MQ_STATUS_INVALID_HANDLE);
  CNDCHK( connection->getIsXA() == PR_TRUE, MQ_ILLEGAL_CLOSE_XA_CONNECTION);

  // Close the connection
  ERRCHK( connection->close() );

  // Release our pointer to the connection.
  releaseHandledObject(connection);
    
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(connection);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQCloseXAConnection(const MQConnectionHandle connectionHandle)
{
  static const char FUNCNAME[] = "MQCloseXAConnection";
  MQError errorCode = MQ_SUCCESS;
  Connection * connection = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);

  // Convert connectionHandle to a Connection pointer
  connection = (Connection*)getHandledObject(connectionHandle.handle,
                                             CONNECTION_OBJECT);
  CNDCHK( connection == NULL, MQ_STATUS_INVALID_HANDLE);

  // Close the connection
  ERRCHK( connection->close() );

  // Release our pointer to the connection.
  releaseHandledObject(connection);

  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(connection);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQFreeConnection(MQConnectionHandle connectionHandle)
{
  MQError errorCode = MQ_SUCCESS;
  Connection * connection = NULL;

  CLEAR_ERROR_TRACE(PR_TRUE);                                                        

  // Convert connectionHandle to a Connection pointer
  connection = (Connection*)getHandledObject(connectionHandle.handle, 
                                             CONNECTION_OBJECT);
  CNDCHK( connection == NULL, MQ_STATUS_INVALID_HANDLE);

  // make sure the connection was closed first
  CNDCHK( !connection->getIsConnectionClosed(), MQ_STATUS_CONNECTION_NOT_CLOSED );
  
  // Release our pointer to the connection.
  releaseHandledObject(connection);
  
  // Delete the connection
  freeHandledObject(connectionHandle.handle, CONNECTION_OBJECT);

  CLEAR_ERROR_TRACE(PR_TRUE);                                                        
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(connection);
  CLEAR_ERROR_TRACE(PR_TRUE);                                                        
  RETURN_STATUS( errorCode );
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQStartConnection(const MQConnectionHandle connectionHandle)
{
  static const char FUNCNAME[] = "MQStartConnection";
  MQError errorCode = MQ_SUCCESS;
  Connection * connection = NULL;
  
  CLEAR_ERROR_TRACE(PR_FALSE);

  // Convert connectionHandle to a Connection pointer
  connection = (Connection*)getHandledObject(connectionHandle.handle, 
                                             CONNECTION_OBJECT);
  CNDCHK( connection == NULL, MQ_STATUS_INVALID_HANDLE);

  // Start the connection
  ERRCHK( connection->start() );
  
  releaseHandledObject(connection);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(connection);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}


/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQStopConnection(const MQConnectionHandle connectionHandle)
{
  static const char FUNCNAME[] = "MQStopConnection";
  MQError errorCode = MQ_SUCCESS;
  Connection * connection = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);
  
  // Convert connectionHandle to a Connection pointer
  connection = (Connection*)getHandledObject(connectionHandle.handle, 
                                             CONNECTION_OBJECT);
  CNDCHK( connection == NULL, MQ_STATUS_INVALID_HANDLE);

  // Start the connection
  ERRCHK( connection->stop() );

  releaseHandledObject(connection);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(connection);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}


EXPORTED_SYMBOL MQStatus
MQGetConnectionProperties(const MQConnectionHandle connectionHandle,
                          MQPropertiesHandle *  propertiesHandle)
{
  static const char FUNCNAME[] = "MQGetConnectionProperties";
  MQError errorCode = MQ_SUCCESS;
  Connection * connection = NULL;
  const Properties * connectionProperties = NULL;
  Properties * clonedProperties = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);

  CNDCHK( propertiesHandle == NULL, MQ_NULL_PTR_ARG );
  propertiesHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Convert connectionHandle to a Connection pointer
  connection = (Connection*)getHandledObject(connectionHandle.handle,
                                             CONNECTION_OBJECT);
  CNDCHK( connection == NULL, MQ_STATUS_INVALID_HANDLE);

  ERRCHK( connection->getProperties(&connectionProperties) );


  clonedProperties = connectionProperties->clone();
  CNDCHK( clonedProperties == NULL, MQ_OUT_OF_MEMORY );
  ERRCHK( clonedProperties->getInitializationError() );

  // Set the output handle
  ERRCHK( clonedProperties->setIsExported(PR_TRUE) );
  propertiesHandle->handle = clonedProperties->getHandle();

  releaseHandledObject(connection);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  DELETE( clonedProperties );
  releaseHandledObject(connection);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus 
MQCreateSession(const MQConnectionHandle connectionHandle,
                  MQBool    isTransacted,
                  MQAckMode acknowledgeMode,
                  MQReceiveMode receiveMode,
                  MQSessionHandle * sessionHandle)
{
  static const char FUNCNAME[] = "MQCreateSession";
  MQError errorCode = MQ_SUCCESS;
  Connection * connection = NULL;
  Session * session = NULL;
  
  CLEAR_ERROR_TRACE(PR_FALSE);                                                        

  CNDCHK( sessionHandle == NULL, MQ_NULL_PTR_ARG );
  sessionHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Convert connectionHandle to a Connection pointer
  connection = (Connection*)getHandledObject(connectionHandle.handle, 
                                             CONNECTION_OBJECT);
  CNDCHK( connection == NULL, MQ_STATUS_INVALID_HANDLE);

  AckMode mode;
  errorCode = mqAckModeToAckMode(acknowledgeMode, &mode);
  if (errorCode == MQ_INVALID_ACKNOWLEDGE_MODE && isTransacted == MQ_TRUE) { 
    mode = SESSION_TRANSACTED;
    errorCode = MQ_SUCCESS;
  }
  ERRCHK( errorCode );

  // Create a session
  if (receiveMode == MQ_SESSION_SYNC_RECEIVE) {
  ERRCHK( connection->createSession(isTransacted, mode, SESSION_SYNC_RECEIVE, PR_FALSE, NULL, NULL, NULL, &session) );
  } else if (receiveMode == MQ_SESSION_ASYNC_RECEIVE) {
  ERRCHK( connection->createSession(isTransacted, mode, SESSION_ASYNC_RECEIVE, PR_FALSE, NULL, NULL, NULL, &session) );
  } else {
  ERRCHK( MQ_INVALID_RECEIVE_MODE );
  }

  // Make the session a valid handle
  session->setIsExported(PR_TRUE);
  sessionHandle->handle = session->getHandle();

  releaseHandledObject(connection);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(connection);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);                                           
  RETURN_STATUS( errorCode );
}


EXPORTED_SYMBOL MQStatus
MQCreateXASession(const MQConnectionHandle    connectionHandle,
                  MQReceiveMode               receiveMode,
                  MQMessageListenerBAFunc beforeMessageListener,
                  MQMessageListenerBAFunc afterMessageListener,
                  void *                callbackData,
                  MQSessionHandle *           sessionHandle)
{
  static const char FUNCNAME[] = "MQCreateXASession";
  MQError errorCode = MQ_SUCCESS;
  Connection * connection = NULL;
  Session * session = NULL;
  
  CLEAR_ERROR_TRACE(PR_FALSE);                                                        

  CNDCHK( sessionHandle == NULL, MQ_NULL_PTR_ARG );
  sessionHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  // Convert connectionHandle to a Connection pointer
  connection = (Connection*)getHandledObject(connectionHandle.handle, 
                                             CONNECTION_OBJECT);
  CNDCHK( connection == NULL, MQ_STATUS_INVALID_HANDLE);
  CNDCHK( connection->getIsXA() == PR_FALSE, MQ_NOT_XA_CONNECTION );

  // Create a session
  if (receiveMode == MQ_SESSION_SYNC_RECEIVE) {
  ERRCHK( connection->createSession(PR_FALSE, AUTO_ACKNOWLEDGE, 
          SESSION_SYNC_RECEIVE, PR_TRUE, NULL, NULL, NULL, &session) );
  } else if (receiveMode == MQ_SESSION_ASYNC_RECEIVE) {
  CNDCHK( beforeMessageListener == NULL, MQ_NULL_PTR_ARG );
  CNDCHK( afterMessageListener == NULL, MQ_NULL_PTR_ARG );
  ERRCHK( connection->createSession(PR_FALSE, AUTO_ACKNOWLEDGE,
          SESSION_ASYNC_RECEIVE, PR_TRUE, beforeMessageListener, afterMessageListener, callbackData, &session) );
  } else {
  ERRCHK( MQ_INVALID_RECEIVE_MODE );
  }

  // Make the session a valid handle
  session->setIsExported(PR_TRUE);
  sessionHandle->handle = session->getHandle();

  releaseHandledObject(connection);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(connection);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);                                           
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQGetMetaData(const MQConnectionHandle connectionHandle,
              MQPropertiesHandle * propertiesHandle)
{
  static const char FUNCNAME[] = "MQGetMetaData";
  MQError errorCode = MQ_SUCCESS;
  Connection * connection = NULL;
  Properties * metaProps = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);                                                        

  NULLCHK( propertiesHandle );
  propertiesHandle->handle = (MQInt32)HANDLED_OBJECT_INVALID_HANDLE;

  connection = (Connection*)getHandledObject(connectionHandle.handle,
                                       CONNECTION_OBJECT);
  CNDCHK( connection == NULL, MQ_STATUS_INVALID_HANDLE );

  // Get the message headers
  ERRCHK( connection->getMetaData(&metaProps) );
  ASSERT( metaProps != NULL );

  // Set the output handle
  metaProps->setIsExported(PR_TRUE);
  propertiesHandle->handle = metaProps->getHandle();

  releaseHandledObject(connection);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(connection);
  MQ_ERROR_TRACE(FUNCNAME, errorCode);                                                        
  RETURN_STATUS( errorCode );

}


MQError 
mqAckModeToAckMode(MQAckMode mqmode, AckMode * mode)
{
  MQError errorCode = MQ_SUCCESS;

  CNDCHK( mode == NULL, MQ_NULL_PTR_ARG );

  switch(mqmode) {
  case MQ_AUTO_ACKNOWLEDGE:
    *mode = AUTO_ACKNOWLEDGE;
    break;
  case MQ_CLIENT_ACKNOWLEDGE:
    *mode = CLIENT_ACKNOWLEDGE;
    break;
  case MQ_DUPS_OK_ACKNOWLEDGE:
    *mode = DUPS_OK_ACKNOWLEDGE;
    break;
  default:
    ERRCHK( MQ_INVALID_ACKNOWLEDGE_MODE );
  }

Cleanup:
  return errorCode;
}

