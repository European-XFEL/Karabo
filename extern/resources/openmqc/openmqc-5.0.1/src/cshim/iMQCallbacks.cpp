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
 * @(#)iMQCallbacks.cpp	1.19 11/09/07
 */ 

#include "mqcallbacks-priv.h"
#include "iMQCallbackUtils.hpp"
#include "../client/MessageConsumer.hpp"
#include "../client/Connection.hpp"
#include "../client/Session.hpp"
#include "shimUtils.hpp"

//
// These methods are used to install callbacks 
//

MQError
invokeMessageListener(const MessageConsumer *   consumer,
                      MQMessageListenerFunc     messageListener,
                      void *                    callbackData,
                      Message *                 message, PRBool *invoked)
{
  MQError errorCode = MQ_SUCCESS;

  MQConsumerHandle consumerHandle;
  MQSessionHandle  sessionHandle;
  MQMessageHandle  messageHandle;
  Session *        session;

  ASSERT( invoked != NULL );
  NULLCHK( invoked );
  *invoked = PR_FALSE;

  ASSERT( consumer != NULL );
  NULLCHK( consumer );
  ASSERT( messageListener != NULL );
  NULLCHK( messageListener );
  ASSERT( message != NULL );
  NULLCHK( message );

  consumerHandle.handle = consumer->getHandle();
  
  session = consumer->getSession();
  ASSERT( session != NULL );
  NULLCHK( session );
  sessionHandle.handle = session->getHandle();

  messageHandle.handle = message->getHandle();

  *invoked = PR_TRUE;
  errorCode = (messageListener)(sessionHandle, consumerHandle, messageHandle, callbackData);

Cleanup:
  CLEAR_ERROR_TRACE( PR_FALSE );
  return errorCode;
}


MQError
invokeMessageListenerBA(const MessageConsumer *   consumer,
                        MQMessageListenerBAFunc   messageListenerBA,
                        void *                    callbackData,
                        const Message *           message, 
                        MQError                   mqerror, 
                        PRBool                    *invoked)
{
  MQError errorCode = MQ_SUCCESS;

  MQConsumerHandle consumerHandle;
  MQSessionHandle  sessionHandle;
  MQMessageHandle  messageHandle;
  Session *        session;

  ASSERT( invoked != NULL );
  NULLCHK( invoked );
  *invoked = PR_FALSE;

  ASSERT( consumer != NULL );
  NULLCHK( consumer );
  ASSERT( messageListenerBA != NULL );
  NULLCHK( messageListenerBA );
  ASSERT( message != NULL );
  NULLCHK( message );

  consumerHandle.handle = consumer->getHandle();

  session = consumer->getSession();
  ASSERT( session != NULL );
  NULLCHK( session );
  sessionHandle.handle = session->getHandle();

  messageHandle.handle = message->getHandle();

  *invoked = PR_TRUE;
  errorCode = (messageListenerBA)(sessionHandle, consumerHandle, 
                                  messageHandle, mqerror, callbackData);

Cleanup:
  CLEAR_ERROR_TRACE( PR_FALSE );
  return errorCode;
}


EXPORTED_SYMBOL MQStatus
MQSetMessageArrivedFunc(const MQConsumerHandle     consumerHandle,
                        MQMessageArrivedFunc       messageCallback,
                        void *                     callbackData)
{
  static const char FUNCNAME[] = "MQSetMessageArrivedFunc";
  MQError errorCode = MQ_SUCCESS;
  MessageConsumer * consumer = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);
                                                                  
  // Convert consumerHandle to a MessageConsumer pointer
  consumer = (MessageConsumer*)getHandledObject(consumerHandle.handle, 
                                                MESSAGE_CONSUMER_OBJECT);
  CNDCHK( consumer == NULL, MQ_STATUS_INVALID_HANDLE);

  ERRCHK( consumer->setMessageArrivedCallback(messageCallback, callbackData) );

  releaseHandledObject(consumer);
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  releaseHandledObject(consumer);
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}



//
// These methods are used by the classes in the library to call out
//


/*
 *
 */
MQError
invokeMessageArrivedCallback(const MessageConsumer * consumer,
                             MQMessageArrivedFunc callback,
                             void* callbackData)
{
  ASSERT( consumer != NULL );
  ASSERT( consumer->getSession() != NULL );
  if ((consumer == NULL)             || 
      (consumer->getSession() == NULL) || 
      (callback == NULL)) 
  {
    return MQ_NULL_PTR_ARG;
  }

  // Invoke the callback
  MQConsumerHandle consumerHandle;
  MQSessionHandle sessionHandle;
  consumerHandle.handle = consumer->getHandle();
  sessionHandle.handle = consumer->getSession()->getHandle();
  (callback)(sessionHandle, consumerHandle, callbackData);
  CLEAR_ERROR_TRACE( PR_FALSE );

  return MQ_SUCCESS;
}

/*
 *
 */
MQBool
invokeCreateThreadCallback(MQThreadFunc startFunc,
                           void * arg,
                           MQCreateThreadFunc callback,
                           void * callbackData)
{
  MQBool success = MQ_FALSE;
  ASSERT( callback != NULL );
  if (callback == NULL) {
    return MQ_FALSE;
  }

  // invoke the callback
  success = (callback)(startFunc, arg, callbackData);

  CLEAR_ERROR_TRACE( PR_FALSE );
  return success;
}

/*
 *
 */
void 
invokeExceptionListenerCallback(const Connection * const connection,
                                MQError exceptionError,
                                MQConnectionExceptionListenerFunc callback,
                                void * callbackData)
{
  ASSERT( connection != NULL );
  ASSERT( callback != NULL );
  if ((connection == NULL) || (callback == NULL)) {
    return;
  }

  // Invoke the callback
  MQStatus status;
  status.errorCode = exceptionError;
  MQConnectionHandle connectionHandle;
  connectionHandle.handle = connection->getHandle();
  (callback)(connectionHandle, status, callbackData);
  CLEAR_ERROR_TRACE( PR_FALSE );
}


void 
invokeLoggingCallback(const PRInt32 severity,
                      const PRInt32 logCode,
                      ConstMQString logMessage,
                      const PRInt64 timeOfMessage,
                      const PRInt64 connectionID,
                      ConstMQString filename,
                      const PRInt32 fileLineNumber,
                      MQLoggingFunc callback,
                      void* callbackData)
{
  /* caller checks callback != NULL before calling this function
   * still has chance that callback becomes NULL because application 
   * set callback func after the check and  before at here - this should
   * be a rare case - application should set log callback at begining  */

  if (callback == NULL) {
    return;
  }

  // invoke the callback
  (callback)((MQLoggingLevel)severity, (MQInt32)logCode, logMessage, 
             (MQInt64)timeOfMessage, (MQInt64)connectionID, 
             filename, (MQInt32)fileLineNumber, callbackData);
  CLEAR_ERROR_TRACE( PR_FALSE );
}
