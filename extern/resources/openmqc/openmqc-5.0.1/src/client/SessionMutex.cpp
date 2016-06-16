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
 * @(#)SessionMutex.cpp	1.3 06/26/07
 */ 

#include "SessionMutex.hpp"

/*
 *
 */ 
SessionMutex::SessionMutex()
{
  this->owner = NULL; 
}

/*
 *
 */
SessionMutex::~SessionMutex()
{
  this->owner = NULL;
}


/*
 * Call unlock() unless if lockedByMe returns true
 */
MQError
SessionMutex:: trylock(PRBool * lockedByMe) 
{
  return trylock(PR_GetCurrentThread(), lockedByMe);
}


MQError
SessionMutex:: trylock(PRThread * me, PRBool * lockedByMe)
{
  *lockedByMe = PR_FALSE;

  monitor.enter();
  if (owner == NULL) {
    owner = me;
    monitor.exit();
    *lockedByMe = PR_TRUE;
    return MQ_SUCCESS;
  }
  if (owner != me) {
    monitor.exit();
    return MQ_CONCURRENT_ACCESS;
  }

  monitor.exit();

  return MQ_SUCCESS;
}


MQError
SessionMutex::unlock() 
{
  return unlock(PR_GetCurrentThread());
}

MQError
SessionMutex::unlock(PRThread * me) 
{
  monitor.enter();
  if (owner == me) {
    owner = NULL;
    monitor.notifyAll();
    monitor.exit();
    return MQ_SUCCESS;
  }

  monitor.exit();
  return MQ_CONCURRENT_NOT_OWNER;
}


/*
 * Call unlock() unless if lockedByMe returns true
 */
MQError
SessionMutex::lock(PRUint32 timeoutMicroSeconds, PRBool * lockedByMe)
{

  MQError errorCode = MQ_SUCCESS;

  PRIntervalTime intervalBefore = PR_INTERVAL_NO_WAIT;
  PRIntervalTime intervalAfter = PR_INTERVAL_NO_WAIT;
  PRIntervalTime intervalWaited = PR_INTERVAL_NO_WAIT;
  PRIntervalTime newTimeout = PR_INTERVAL_NO_WAIT;
  PRBool firstTimeout = PR_TRUE;

  PRIntervalTime timeout = PR_INTERVAL_NO_WAIT;
  PRThread * me = PR_GetCurrentThread();

  if ((timeoutMicroSeconds == PR_INTERVAL_NO_WAIT) ||
      (timeoutMicroSeconds == PR_INTERVAL_NO_TIMEOUT))
  {
    timeout = timeoutMicroSeconds;
  } else {
    timeout = PR_MicrosecondsToInterval(timeoutMicroSeconds);
  }

  newTimeout = timeout;
  *lockedByMe = PR_FALSE;

  monitor.enter();

  while (owner != NULL && owner != me) {

    if (timeout == PR_INTERVAL_NO_WAIT) {
      errorCode = MQ_CONCURRENT_ACCESS; 
      break;
    }

    if (timeout != PR_INTERVAL_NO_TIMEOUT && firstTimeout == PR_FALSE) {
      if ((intervalAfter - intervalBefore) > 0) {
        intervalWaited = intervalAfter - intervalBefore;
      } else {
        intervalWaited =  (PR_INTERVAL_NO_TIMEOUT - intervalBefore) + intervalAfter;
      }
      if (intervalWaited >= newTimeout)  {
        errorCode = MQ_TIMEOUT_EXPIRED;
        break;
      }
      newTimeout -= intervalWaited;
    }

    if (timeout != PR_INTERVAL_NO_TIMEOUT) {
      intervalBefore =PR_IntervalNow();
    }

    monitor.wait(newTimeout);

    if (timeout != PR_INTERVAL_NO_TIMEOUT) {
      intervalAfter = PR_IntervalNow();
      firstTimeout = PR_FALSE;
    }

  } //while

  if (owner == NULL) {
    owner = me;
    *lockedByMe = PR_TRUE;
  }

  monitor.exit();

  return errorCode;

}
