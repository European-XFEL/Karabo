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
 * @(#)ReceiveQueue.hpp	1.9 06/26/07
 */ 

#ifndef RECEIEVEQUEUE_H
#define RECEIEVEQUEUE_H

#include <nspr.h>
#include "../basictypes/Object.hpp"
#include "../basictypes/Monitor.hpp"
#include "../containers/ObjectVector.hpp"

//#ifndef PASSWORD_DECENTRAL
class MessageConsumer;
//#endif


/**
 * This class stores a queue of iMQ packets.  ReadChannel::dispatch enqueues
 * packets and the ProtocolHandler or receiver dequeues packets.  
 *
 * @see ReadChannel::dispatch
 * @see MessageConsumer::receive
 */
class ReceiveQueue : public Object {
private:
  /**
   * A queue to hold the packets.
   */
  ObjectVector  msgQueue;

  /**
   * True iff the ReceiveQueue is closed.
   */
  PRBool        isClosed;

  PRBool        isStopped;

  /**
   * True iff the receiver is in the process of receiving a message
   */
  PRBool        receiveInProgress;

  /**
   * Ensures synchronous access to the packet queue.
   */
  Monitor       monitor;

  /** for sync-receiving message arrival notification,  NULL otherwise  */
  MessageConsumer * syncReceiveConsumer;

  /**
   * references and closeWaited are for MessageConsumer::close */

  /** number of callers currently in dequeueWait() */
  PRInt32      references;
  /** if true a close() call is waiting for references == 0 */
  PRBool       closeWaited;

  /**
   * Initializes all member variables.
   */
  void init();

  /** If a thread is accessing the queue, then this method blocks
      until it is done */
  void waitUntilReceiveIsDone();
  
         
public:

  /** Constructor */
  ReceiveQueue();

  ReceiveQueue(PRUint32 initialSize);

  /** Destructor */
  ~ReceiveQueue();

  /** Waits for an object to be enqueued, and then returns that
   *  object.  NULL is returned if there was an exception
   *  @return the next object in the queue */
  Object * dequeueWait();

  /** Waits for the specified interval for an object to be enqueued,
   *  and then returns that object.  NULL is returned if there was an
   *  exception
   *  @param timeoutMicroSeconds the number of microseconds to wait for
   *  an object to be enqueued.
   *  @return the next object in the queue */
  Object * dequeueWait(const PRUint32 timeoutMicroSeconds);

  /** Enqueues an object and notifies any thread that has called
      dequeueWait */
  iMQError  enqueueNotify(Object * const obj);
  
  /** Called when the receiving thread, the thread that calls dequeue
   *  or dequeueWait is done with receiving a message */
  void receiveDone();

  void stop();
  void start();
  void reset();

  /** Enqueues an object with no special synchronization */
  iMQError  enqueue(Object * const obj);

  /** Dequeues an object with no special synchronization */
  Object *  dequeue();

  /** Close the ReceiveQueue */
  void      close();
  /** wait until references == 0 - called by MessageConsumer::close() */
  void      close(PRBool wait);

  /** Accessors */
  PRBool    getIsClosed();

  PRUint32 size();
  PRBool isEmpty();

  void setSyncReceiveConsumer(MessageConsumer * consumer);

  
  /** Static test method */
  static    iMQError test();

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  ReceiveQueue(const ReceiveQueue& queue);
  ReceiveQueue& operator=(const ReceiveQueue& queue);

};


#endif // RECEIEVEQUEUE_H
