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
 * @(#)ReceiveQueue.cpp	1.11 06/26/07
 */ 

#include "ReceiveQueue.hpp"
#include "../util/UtilityMacros.h"
#include "../util/LogUtils.hpp"
#include "../basictypes/Integer.hpp"


#include "MessageConsumer.hpp"

/*
 *
 */
ReceiveQueue::ReceiveQueue()
{
  CHECK_OBJECT_VALIDITY();

  init();
}

/*
 *
 */
ReceiveQueue::ReceiveQueue(PRUint32 intialSize):msgQueue(intialSize)
{
  CHECK_OBJECT_VALIDITY();

  init();
}

/*
 *
 */
ReceiveQueue::~ReceiveQueue()
{
  CHECK_OBJECT_VALIDITY();

  LOG_FINER(( CODELOC, RECEIVE_QUEUE_LOG_MASK, NULL_CONN_ID, IMQ_SUCCESS,
              "ReceiveQueue::~ReceiveQueue() called on 0x%p", this ));
}


/*
 *
 */
Object * 
ReceiveQueue::dequeueWait()
{
  CHECK_OBJECT_VALIDITY();

  return dequeueWait(PR_INTERVAL_NO_TIMEOUT);
}


/*
 *
 */
Object * 
ReceiveQueue::dequeueWait(const PRUint32 timeoutMicroSeconds)
{
  CHECK_OBJECT_VALIDITY();

  PRIntervalTime prTimeout = microSecondToIntervalTimeout(timeoutMicroSeconds);
  PRIntervalTime newPrTimeout = prTimeout;
  PRIntervalTime intervalBefore = PR_INTERVAL_NO_WAIT;
  PRIntervalTime intervalAfter = PR_INTERVAL_NO_WAIT;
  PRIntervalTime intervalWaited = PR_INTERVAL_NO_WAIT;
  PRBool firstTimeout = PR_TRUE;

  Object * object = NULL;

  monitor.enter();
  this->references++;
    if (this->isClosed) {
      this->references--;
      monitor.exit();
      return NULL;
    }
    
    // Wait for a packet to arrive.
    while ((msgQueue.size() == 0) || isStopped) {

      if (prTimeout == PR_INTERVAL_NO_WAIT) {
        break;
      }
      if (prTimeout != PR_INTERVAL_NO_TIMEOUT && firstTimeout == PR_FALSE) {
        if ((intervalAfter - intervalBefore) > 0) { 
          intervalWaited = intervalAfter - intervalBefore;
        } else {
          intervalWaited =  (PR_INTERVAL_NO_TIMEOUT - intervalBefore) + intervalAfter;
        }
        if (intervalWaited >= newPrTimeout) {
          break;
        }
        newPrTimeout -= intervalWaited;
      }
      LOG_FINEST(( CODELOC, RECEIVE_QUEUE_LOG_MASK, NULL_CONN_ID, IMQ_SUCCESS,
                   "ReceiveQueue::dequeueWait() calling monitor.wait(timeout=%d)", prTimeout ));
      if (prTimeout != PR_INTERVAL_NO_TIMEOUT) {
        intervalBefore =PR_IntervalNow();
      }
      monitor.wait(newPrTimeout);
      if (prTimeout != PR_INTERVAL_NO_TIMEOUT) {
        intervalAfter = PR_IntervalNow();
        firstTimeout = PR_FALSE;
      }
      LOG_FINEST(( CODELOC, RECEIVE_QUEUE_LOG_MASK, NULL_CONN_ID, IMQ_SUCCESS,
          "ReceiveQueue::dequeueWait() returning from  monitor.wait(timeout=%d),isStopped = %d, isClosed = %d, isEmpty = %d, total timeout=%d",
                   newPrTimeout, isStopped, isClosed, isEmpty(), prTimeout ));

    } //while

    if (!isStopped && !isClosed && !isEmpty()) {
      receiveInProgress = PR_TRUE;
      object = dequeue();
      LOG_FINEST(( CODELOC, RECEIVE_QUEUE_LOG_MASK, NULL_CONN_ID, IMQ_SUCCESS,
                   "ReceiveQueue::dequeueWait() dequeued object = 0x%p", object ));
    }
  
  this->references--;
  if (closeWaited) {
    monitor.notifyAll();
  }
  monitor.exit();
  
  return object;
}

/*
 *
 */
void 
ReceiveQueue::receiveDone()
{
  CHECK_OBJECT_VALIDITY();

  monitor.enter();
  this->receiveInProgress = PR_FALSE;
  monitor.notifyAll();
  monitor.exit();
}


/*
 *
 */
void
ReceiveQueue::stop()
{
  CHECK_OBJECT_VALIDITY();

  LOG_FINEST(( CODELOC, RECEIVE_QUEUE_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
               "ReceiveQueue::stopping (0x%p)", this ));

  monitor.enter();
  this->isStopped = PR_TRUE;
  waitUntilReceiveIsDone();
  monitor.exit();

  LOG_FINEST(( CODELOC, RECEIVE_QUEUE_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
               "ReceiveQueue::stopped (0x%p)", this ));

}


/*
 *
 */
void
ReceiveQueue::start()
{
  CHECK_OBJECT_VALIDITY();

  LOG_FINEST(( CODELOC, RECEIVE_QUEUE_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
               "ReceiveQueue::starting (0x%p)", this ));

  monitor.enter();
  this->isStopped = PR_FALSE;
  monitor.notifyAll();
  monitor.exit();

  LOG_FINEST(( CODELOC, RECEIVE_QUEUE_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
               "ReceiveQueue::started (0x%p)", this ));
}

/*
 *
 */
void
ReceiveQueue::waitUntilReceiveIsDone()
{
  CHECK_OBJECT_VALIDITY();

  monitor.enter();
  while (isStopped && receiveInProgress) {
    monitor.wait();
  }
  monitor.exit();
}


// These methods used to be part of SessionQueue before it was merged
// with ReceiveQueue.


/*
 *
 */
void
ReceiveQueue::reset()
{
  CHECK_OBJECT_VALIDITY();

  monitor.enter();
    msgQueue.reset();
  monitor.exit();
}


/*
 *
 */
PRUint32
ReceiveQueue::size()
{
  CHECK_OBJECT_VALIDITY();

  PRUint32 size = 0;

  monitor.enter();
    size = msgQueue.size();
  monitor.exit();

  return size;
}

/*
 *
 */
PRBool
ReceiveQueue::isEmpty()
{
  CHECK_OBJECT_VALIDITY();

  monitor.enter();
  if (msgQueue.size() == 0) {
    monitor.exit();
    return PR_TRUE;
  }
  monitor.exit();
  return PR_FALSE;
}


/**
 * Enqueues an object in the queue with no special synchronization.
 * @param obj new object to be enqueued
 */
iMQError
ReceiveQueue::enqueue(Object * const obj) 
{
  CHECK_OBJECT_VALIDITY();

  // We don't test for isClosed because we must enqueue NULL to wakeup
  // receivers.
  monitor.enter();
    iMQError errorCode = msgQueue.add(obj);
  monitor.exit();
  LOG_FINEST(( CODELOC, RECEIVE_QUEUE_LOG_MASK, NULL_CONN_ID, IMQ_SUCCESS,
               "ReceiveQueue::enqueue(0x%p) %s",
               obj, (errorCode == IMQ_SUCCESS) ? "succeeded" : "FAILED" ));

  return errorCode;
}

/*
 *
 */
iMQError  
ReceiveQueue::enqueueNotify(Object * const obj)
{
  CHECK_OBJECT_VALIDITY();
  iMQError errorCode = IMQ_SUCCESS;

  // We don't test for isClosed because we must enqueue NULL to wakeup
  // receivers.
  monitor.enter();
    errorCode = enqueue(obj);
    LOG_FINEST(( CODELOC, RECEIVE_QUEUE_LOG_MASK, NULL_CONN_ID, IMQ_SUCCESS,
                 "ReceiveQueue::enqueueNotify(0x%p), notifying consumer",
                 obj, (errorCode == IMQ_SUCCESS) ? "succeeded" : "FAILED" ));

    monitor.notifyAll();

    // If syncReceiveConsumer is not NULL, notify it that a message has arrived
    if ((errorCode == IMQ_SUCCESS) && (syncReceiveConsumer != NULL)) {
      LOG_FINEST(( CODELOC, RECEIVE_QUEUE_LOG_MASK, NULL_CONN_ID, IMQ_SUCCESS,
                   "ReceiveQueue::enqueueNotify(0x%p), notifying consumer", obj ));
      syncReceiveConsumer->messageEnqueued();
    }

  monitor.exit();
  return errorCode;
}

 /**
  * Dequeues an element from the queue without any special synchronization.
  * @return dequeued object, or null if empty queue
  */
Object *
ReceiveQueue::dequeue() 
{
  CHECK_OBJECT_VALIDITY();

  Object * obj = NULL;
  
  monitor.enter();
    if (this->isClosed) {
      monitor.exit();
      return NULL;
    }
    iMQError errorCode = msgQueue.remove(0, (void**)&obj);
    if (errorCode != IMQ_SUCCESS) {
      ASSERT(obj == NULL);
      obj = NULL;
    }
  monitor.exit();
  
  return obj;
}


/*
 *
 */
void
ReceiveQueue::close() 
{
  this->close(PR_FALSE);
}

void
ReceiveQueue::close(PRBool wait) 
{
  CHECK_OBJECT_VALIDITY();

  monitor.enter();
  this->isClosed = PR_TRUE;
  this->isStopped = PR_FALSE; 

  // Wake up anyone who is waiting
  enqueueNotify(NULL);
  if (wait == PR_TRUE && this->references > 0) {
    closeWaited = PR_TRUE;
    LOG_FINEST(( CODELOC, RECEIVE_QUEUE_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
                 "ReceiveQueue::close(0x%p) wait(references=%d)", this, this->references ));
    while(this->references > 0) {
      monitor.wait();
    }
    LOG_FINEST(( CODELOC, RECEIVE_QUEUE_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
                 "ReceiveQueue::close(0x%p) wait(references=%d) done", this, this->references ));
    closeWaited = PR_FALSE;
  }
  monitor.exit();
}


/*
 *
 */
PRBool
ReceiveQueue::getIsClosed() 
{
  CHECK_OBJECT_VALIDITY();

  monitor.enter();
    PRBool isClosedLocal = this->isClosed;
  monitor.exit();

  return isClosedLocal;
}


// SYNCHRONIZATION METHODS

/*
 *
 */
void
ReceiveQueue::init()
{
  CHECK_OBJECT_VALIDITY();

  isClosed          = PR_FALSE;
  isStopped         = PR_FALSE;
  receiveInProgress = PR_FALSE;
  syncReceiveConsumer = NULL;
  closeWaited       = PR_FALSE;
  references         = 0;
}

void
ReceiveQueue::setSyncReceiveConsumer(MessageConsumer * consumer) 
{
  CHECK_OBJECT_VALIDITY();
  this->syncReceiveConsumer = consumer;
}


/*
 *
 */
iMQError 
ReceiveQueue::test()
{
  ReceiveQueue* q = NULL;
  Integer    one(1);
  Integer    two(2);
  Integer    three(3);
  Integer *  result;
  iMQError   errorCode = IMQ_SUCCESS;

  MEMCHK( q = new ReceiveQueue() );
  q->enqueue(&one);
  q->enqueue(&two);

  result = (Integer*)q->dequeue();
  CNDCHK( result != &one, IMQ_RECEIVE_QUEUE_ERROR );

  result = (Integer*)q->dequeue();
  CNDCHK( result != &two, IMQ_RECEIVE_QUEUE_ERROR );

  result = (Integer*)q->dequeue();
  CNDCHK( result != NULL, IMQ_RECEIVE_QUEUE_ERROR );

  q->enqueue(&three);
  result = (Integer*)q->dequeue();
  CNDCHK( result != &three, IMQ_RECEIVE_QUEUE_ERROR );

Cleanup:
  DELETE( q );
  return errorCode;
}

