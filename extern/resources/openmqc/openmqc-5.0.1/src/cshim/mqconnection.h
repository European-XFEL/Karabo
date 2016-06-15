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
 * @(#)mqconnection.h	1.18 10/17/07
 */ 

#ifndef MQ_CONNECTION_H
#define MQ_CONNECTION_H

/*
 * declarations of C interface for connection
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mqtypes.h"
#include "mqcallback-types.h"
#include "mqproperties.h"
#include "mqconnection-props.h"

/**
 * Opens a new connection the broker based on the supplied parameters.
 *
 * @param propertiesHandle a handle to a properties object containing
 *        properties to be used for this connection.  This handle will
 *        be invalid after this function returns.
 * @param username the username to use when connecting to the broker
 * @param password the password to use when connection to the broker
 * @param clientID the client ID to use for the connection or NULL
 * @param exceptionListener the callback function for connection exception
 * @param exceptionCallBackData void * data pointer that to be passed to
 *        the connection exceptionListener function whenever it is called.
 * @param connectionHandle the output parameter that contains the
 *        newly opened connection.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.
 */
EXPORTED_SYMBOL MQStatus 
MQCreateConnection(MQPropertiesHandle                propertiesHandle,
                   ConstMQString                     username,
                   ConstMQString                     password,
                   ConstMQString                     clientID,
                   MQConnectionExceptionListenerFunc exceptionListener,
                   void *                            listenerCallBackData,
                   MQConnectionHandle *              connectionHandle);

/**
 * Get a XA connection
 *
 * @param connectionHandle the output parameter that contains the
 *        newly opened connection.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.
 */
EXPORTED_SYMBOL MQStatus
MQGetXAConnection(MQConnectionHandle *connectionHandle);


/**
 * Get the properties of a connection
 *
 * @param connectionHandle the connection handle
 * @param propertiesHandle the output parameter for the properties
 *        of this connection 
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.
 */
EXPORTED_SYMBOL MQStatus
MQGetConnectionProperties(const MQConnectionHandle connectionHandle,
                          MQPropertiesHandle *  propertiesHandle);


/**
 * Starts the connection to the broker.  This starts (or resumes)
 * message delivery.
 *
 * @param connectionHandle the handle to the connection to start
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQStartConnection(const MQConnectionHandle connectionHandle);

/**
 * Stops the connection to the broker.  This stops the broker
 * from delivering messages until MQStartConnection is called.
 *
 * @param connectionHandle the handle to the connection to stop
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQStopConnection(const MQConnectionHandle connectionHandle);

/**
 * Closes the connection to the broker.  This closes all sessions,
 * producers, and consumers created from this connection.  This will
 * force all threads associated with this connection that are blocking
 * in the library (e.g. a consumer calling MQReceiveMessageWait) to return.
 * This does not actually release all resources associated with the
 * connection.  After all of the application threads associated with
 * this connection and its decendant sessions, producers, consumers,
 * etc. have returned, then the application can call
 * MQFreeConnection to release all resources held by this
 * connection and its decendant sessions, producers, consumers, etc..
 *
 * @param connectionHandle the handle to the connection to close
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQCloseConnection(const MQConnectionHandle connectionHandle);

/**
 * Deletes all resources associated with the connection.  This
 * function can only be called after the connection was closed by
 * calling MQCloseConnection, and after all of the application
 * threads associated with this connection and its decendant sessions,
 * producers, consumers, etc. have returned.  This function MUST NOT
 * be called while an application thread is active in a library
 * function associated with this connection or one of its decendant
 * sessions, producers, or consumers.  MQFreeConnection will
 * release all resources held by the connection and its decendant
 * sessions, producers, consumers, and destinations.  It will not
 * release resources held by messages associated with this connection.
 * Resources held by a message must be explicitly freed by calling
 * MQFreeMessage.
 *
 * @param connectionHandle the handle to the connection to delete
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQFreeConnection(MQConnectionHandle connectionHandle);

/**
 * Creates a new session on this connection with the properties
 * specified by the supplied parameters.
 *
 * @param connectionHandle the handle to the connection on which
 *        to create a session
 * @param isTransacted MQ_TRUE iff the session should be transacted.
 *        This currently must be MQ_FALSE.
 * @param acknowledgeMode the type of acknowledge mode to use for
 *        this transaction.  This currently must be
 *        MQ_CLIENT_ACKNOWLEDGE.
 * @param receiveMode synchronous or asynchronous message receiving
 * @param sessionHandle the output parameter that contains the
 *        newly created session.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQCreateSession(const MQConnectionHandle connectionHandle,
                MQBool                   isTransacted,
                MQAckMode                acknowledgeMode,
                MQReceiveMode            receiveMode,
                MQSessionHandle *        sessionHandle);


/**
 * Creates a new XA session on a XA connection 
 *
 * For MQ_SESSION_SYNC_RECEIVE receiveMode, pass NULL for 
 * beforeMessageListener, afterMessageListener and callbackData.
 *
 * @param connectionHandle the handle to the XA connection on which
 *        to create a session
 * @param receiveMode synchronous or asynchronous message receiving
 * @param beforeMessageListener callback function before asynchronous message
 *                       delivery 
 * @param afterMessageListener callback function after asynchronous message
 *                       delivery 
 * @param callbackData  data pointer to be passed to the beforeDelivery
 *                      and afterDelivery functions
 * @param sessionHandle the output parameter that contains the
 *        newly created session.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus
MQCreateXASession(const MQConnectionHandle    connectionHandle,
                  MQReceiveMode               receiveMode,
                  MQMessageListenerBAFunc     beforeMessageListener,
                  MQMessageListenerBAFunc     afterMessageListener,
                  void *                      callbackData,
                  MQSessionHandle *           sessionHandle);

/**
 * Gets the metadata for this connection. The caller is responsible
 * to free the metadata propertiesHandle by MQFreeProperties()
 *
 * @param connectionHandle the handle to the connection
 * @param propertiesHandle the output parameter for the metadata
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus
MQGetMetaData(const MQConnectionHandle connectionHandle,
              MQPropertiesHandle * propertiesHandle); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MQ_CONNECTION_H */
