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
 * @(#)ProducerFlow.cpp	1.9 06/26/07
 */ 

#include "ProducerFlow.hpp"
#include "../util/UtilityMacros.h"
#include "Message.hpp"


ProducerFlow::ProducerFlow()
{
  CHECK_OBJECT_VALIDITY();

  this->producerID = 0;
  this->producerIDLong.setValue(this->producerID);
  this->chunkBytes = -1;
  this->chunkSize = -1;
  this->sentCount = 0;

  this->references = 0;

  this->isClosed = PR_FALSE;
  this->closeReason = MQ_SUCCESS;

}


ProducerFlow::~ProducerFlow()
{
  CHECK_OBJECT_VALIDITY();
  this->close(this->closeReason);
}

void
ProducerFlow::setProducerID(PRInt64 producerIDArg)
{
  CHECK_OBJECT_VALIDITY();
  this->producerID = producerIDArg;
  this->producerIDLong.setValue(this->producerID);
}


PRInt64
ProducerFlow::getProducerID() const
{
  CHECK_OBJECT_VALIDITY();
  return this->producerID;
}



void
ProducerFlow::setChunkSize(PRInt32 chunkSizeArg) 
{
  CHECK_OBJECT_VALIDITY();
  this->chunkSize = chunkSizeArg;
}

void
ProducerFlow::setChunkBytes(PRInt64 chunkBytesArg) 
{
  CHECK_OBJECT_VALIDITY();
  this->chunkBytes = chunkBytesArg;
}

MQError
ProducerFlow::checkFlowControl(Message *message) 
{
  CHECK_OBJECT_VALIDITY();

  MQError errorCode = MQ_SUCCESS;
  Packet * packet = NULL;
  FlowState flowState = UNDER_LIMIT;

  NULLCHK( message );
  LOG_FINEST(( CODELOC, PRODUCER_FLOWCONTROL_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
                 "Entering ProducerFlow::checkFlowControl(producerID=%s)",
                  producerIDLong.toString()));

  monitor.enter();

  while (this->isClosed == PR_FALSE && (flowState = checkFlowLimit()) == OVER_LIMIT) {
  LOG_FINE(( CODELOC, PRODUCER_FLOWCONTROL_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
                 "ProducerFlow::checkFlowControl(producerID=%s, chunckSize=%d, sentCount=%d) calling wait()",
                 producerIDLong.toString(), chunkSize, sentCount ));

  monitor.wait();
  LOG_FINE(( CODELOC, PRODUCER_FLOWCONTROL_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
                "ProducerFlow::checkFlowControl(producerID=%s) wokeup from wait()",
                 producerIDLong.toString() ));
    
  }
  if (this->isClosed) {
    LOG_FINE(( CODELOC, PRODUCER_FLOWCONTROL_LOG_MASK, NULL_CONN_ID, MQ_PRODUCER_CLOSED,
               "ProducerFlow::checkFlowControl(producerID=%s) wokeup from wait() by close",
               producerIDLong.toString() ));
    monitor.exit();
    return this->closeReason;
  }

  packet = message->getPacket();
  packet->setProducerID(this->producerID);
  packet->setConsumerFlow((flowState == ON_LIMIT));
  if (flowState == ON_LIMIT) {
    LOG_FINE(( CODELOC, PRODUCER_FLOWCONTROL_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
         "ProducerFlow::checkFlowControl(producerID=%s, sentCount=%d) sending last message %s",
          producerIDLong.toString(), 
          sentCount,
          ((SysMessageID *)message->getSystemMessageID())->toString() ));
  }
  sentCount++;
  monitor.exit();

Cleanup:
  return errorCode;
}

/*
 * only called from ReadChannel thread
 */
void
ProducerFlow::resumeFlow(PRInt64 chunkBytesArg, PRInt32 chunkSizeArg)
{
  CHECK_OBJECT_VALIDITY();
  Long oldchunkBytesLong(this->chunkBytes);
  Long newchunkBytesLong(chunkBytesArg);

  LOG_FINEST(( CODELOC, PRODUCER_FLOWCONTROL_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
   "ProducerFlow::resumeFlow(producerID=%s) to chunkBytes=%s, chunkSize=%d from chunkBytes=%s, chunkSize=%d",
    producerIDLong.toString(),
    newchunkBytesLong.toString(), chunkSizeArg,
    oldchunkBytesLong.toString(), this->chunkSize ));

  monitor.enter();
  this->chunkBytes = chunkBytesArg;
  this->chunkSize = chunkSizeArg;
  sentCount = 0;
  monitor.notifyAll();
  monitor.exit();
  
}

/*
 * only called by sender thread while in monitor
 */
FlowState
ProducerFlow::checkFlowLimit()
{
  CHECK_OBJECT_VALIDITY();

  if (chunkSize < 0) return UNDER_LIMIT;
  if (sentCount >= chunkSize) return OVER_LIMIT;
  if (sentCount == chunkSize-1) return ON_LIMIT;
  return UNDER_LIMIT;
}


/**
 * acquireReference/releaseReference are or should only be called from
 * Connection.getProducerFlow/releaseProducerFlow methods under the same
 * Monitor in Connection for calls ProducerFlow.close() and ~ProducerFow().
 * Therefore these two methods are safe not using this->monitor
 */
MQError
ProducerFlow::acquireReference()
{
  if (this->isClosed == PR_TRUE) {
    return this->closeReason;
  }

  ASSERT( references >= 0 );
  references++;

  return MQ_SUCCESS;
}

PRBool
ProducerFlow::releaseReference()
{
  ASSERT( references > 0 );
  references--;
  if (references == 0 && isClosed == PR_TRUE && closeReason == MQ_PRODUCER_CLOSED) {
    return PR_TRUE;
  }
  return PR_FALSE;
}


void
ProducerFlow::close(MQError reason)
{
  CHECK_OBJECT_VALIDITY();

  monitor.enter();
  this->isClosed = PR_TRUE;
  this->closeReason = reason;
  monitor.notifyAll();
  monitor.exit();

}
