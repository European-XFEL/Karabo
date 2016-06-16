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
 * @(#)XASession.cpp	1.2 10/23/07
 */ 

#include "XASession.hpp"
#include "Connection.hpp"

/*
 *
 */
XASession::XASession(Connection * const connectionArg,
                     const ReceiveMode  receiveModeArg,
                     MQMessageListenerBAFunc beforeMessageListenerArg, 
                     MQMessageListenerBAFunc afterMessageListenerArg, 
                     void * callbackDataArg
                     ) : Session(connectionArg, PR_FALSE, AUTO_ACKNOWLEDGE, receiveModeArg) 
{
  CHECK_OBJECT_VALIDITY();

  ASSERT (connectionArg->getIsXA() == PR_TRUE ); 
  this->isXA = PR_TRUE; 
  this->ackMode = SESSION_TRANSACTED; 
  this->transactionID = LL_Zero();
  this->xidIndex = mq_getXidIndex();
  this->beforeMessageListener = NULL;
  this->afterMessageListener = NULL;
  this->baMLCallbackData = NULL;
  if (this->receiveMode == SESSION_ASYNC_RECEIVE) {
      this->beforeMessageListener = beforeMessageListenerArg;
      this->afterMessageListener = afterMessageListenerArg;
      this->baMLCallbackData = callbackDataArg;
  }
  LOG_INFO(( CODELOC, XA_SWITCH_LOG_MASK, connectionArg->id(), MQ_SUCCESS,
             "XASession (0x%p) created.", this ));
  return;
}


MQMessageListenerBAFunc
XASession::getBeforeMessageListenerFunc()
{
  CHECK_OBJECT_VALIDITY();
  return this->beforeMessageListener;
  
}


MQMessageListenerBAFunc
XASession::getAfterMessageListenerFunc()
{
  CHECK_OBJECT_VALIDITY();
  return this->afterMessageListener;
}


void *
XASession::getMessageListenerBACallbackData()
{
  CHECK_OBJECT_VALIDITY();
  return this->baMLCallbackData;
}


MQError
XASession::writeJMSMessage(Message * const message, PRInt64 producerID)
{
  CHECK_OBJECT_VALIDITY();

  if (this->isClosed) return MQ_SESSION_CLOSED;

  if (this->isXA == PR_TRUE) {
    MQXID *xid = (MQXID *) PR_GetThreadPrivate(this->xidIndex);
    if (xid == NULL) { 
      return MQ_THREAD_OUTSIDE_XA_TRANSACTION;
    }
    if (LL_IS_ZERO(xid->transactionID) != 0) {
      return MQ_XA_SESSION_NO_TRANSATION;
    }
    this->transactionID = xid->transactionID;
    {
      Long transactionIDLong(this->transactionID);
      LOG_FINE(( CODELOC, XA_SWITCH_LOG_MASK, this->connection->id(), MQ_SUCCESS,
                "XASession::writeJMSMessage with transactionID=%s, in XASession (0x%p)",
                 transactionIDLong.toString(), this ));
    }
  }
  return Session::writeJMSMessage(message, producerID);
}

MQError
XASession::acknowledge(Message * message, PRBool fromMessageListener)
{
  CHECK_OBJECT_VALIDITY();

  if (this->isClosed)  return MQ_SESSION_CLOSED;

  if (this->isXA == PR_TRUE) {
    MQXID *xid = (MQXID *) PR_GetThreadPrivate(this->xidIndex);
    if (xid == NULL) {
      return MQ_THREAD_OUTSIDE_XA_TRANSACTION;
    }
    if (LL_IS_ZERO(xid->transactionID) != 0) {
      return MQ_XA_SESSION_NO_TRANSATION;
    }
    this->transactionID = xid->transactionID;
    {
      Long transactionIDLong(this->transactionID);
      LOG_FINE(( CODELOC, XA_SWITCH_LOG_MASK, this->connection->id(), MQ_SUCCESS,
                "XASession::acknowledge with transactionID=%s, in XASession (0x%p)",
                 transactionIDLong.toString(), this ));
    }
  }

  return Session::acknowledge(message, fromMessageListener);
}


/*
 *
 */
XASession::~XASession()
{
  CHECK_OBJECT_VALIDITY();
}

