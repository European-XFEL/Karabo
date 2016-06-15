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
 * @(#)PingTimer.cpp	1.7 06/26/07
 */ 

#include "PingTimer.hpp"
#include "Connection.hpp"
#include "../util/UtilityMacros.h"
#include "../util/LogUtils.hpp"

/*
 * When the C client supports consumer-based flowcontrol which would need
 * a thread, then this pinging work should be done by that thread so that   
 * to reduce the number of threads in the client runtime.
 *
 */
PingTimer::PingTimer(Connection * const connectionArg)
{
  CHECK_OBJECT_VALIDITY();
  MQError errorCode = MQ_SUCCESS;

  this->init();
  ASSERT( connectionArg != NULL );
  this->connection = connectionArg;
  NULLCHK( this->connection );

  this->connectionID = this->connection->id();

  ASSERT( connectionArg->getPingIntervalSec() > 0 ); 
  this->pingInterval = microSecondToIntervalTimeout(connectionArg->getPingIntervalSec()*1000*1000);

  monitor.enter();
  errorCode = this->connection->startThread(this);
  if (errorCode == MQ_SUCCESS) {
    this->isAlive = PR_TRUE;
  }
  monitor.exit();

Cleanup:
  this->initializationError = errorCode;
}

/*
 *
 */
PingTimer::~PingTimer()
{
  CHECK_OBJECT_VALIDITY();
  LOG_FINE(( CODELOC, PROTOCOL_HANDLER_LOG_MASK, this->connectionID, MQ_SUCCESS,
             "PingTimer::~PingTimer() called" ));

  ASSERT( this->exit );
  
  this->init();
}

/*
 * Connection::openConnection calls this method to make sure that the
 * constructor was able to successfully create the reader thread.
 */
MQError
PingTimer::getInitializationError() const
{
  return this->initializationError;
}

/*
 *
 */
void
PingTimer::init()
{
  CHECK_OBJECT_VALIDITY();

  this->connection           = NULL;
  this->connectionID         = NULL_CONN_ID;
  this->pingInterval         = PR_INTERVAL_NO_TIMEOUT;
  this->isAlive              = PR_FALSE;
  this->exit                 = PR_FALSE;
  this->initializationError  = MQ_SUCCESS; 
  this->pingThread = NULL;
}


/*
 *
 */
void
PingTimer::run()
{
  CHECK_OBJECT_VALIDITY();
  MQError errorCode = MQ_SUCCESS;

  this->pingThread = PR_GetCurrentThread();

  monitor.enter();
  if (this->connection == NULL) {
    this->isAlive = PR_FALSE;
    monitor.notifyAll();
    monitor.exit();
    return;
  }
  monitor.exit();

  LOG_FINE(( CODELOC, PROTOCOL_HANDLER_LOG_MASK, this->connectionID, MQ_SUCCESS,
             "PingTimer:: started;  isAlive=%d "
             "exit=%d ", this->isAlive, this->exit ));

   monitor.enter();

   while(!this->exit) {

     LOG_FINEST(( CODELOC, PROTOCOL_HANDLER_LOG_MASK, this->connectionID, MQ_SUCCESS,
                "PingTimer calling wait() ..." ));

     monitor.wait(this->pingInterval);

     LOG_FINEST(( CODELOC, PROTOCOL_HANDLER_LOG_MASK, this->connectionID, MQ_SUCCESS,
                "PingTimer wakeup" ));

     if (this->exit == PR_FALSE) {
       errorCode = connection->ping(); //error is logged in ProtocolHandler
     }

   } //while

  CLEAR_ERROR_TRACE(PR_TRUE);

  this->isAlive = PR_FALSE;
  monitor.notifyAll();
  monitor.exit();
  return;
}

void
PingTimer::terminate()
{
  CHECK_OBJECT_VALIDITY();

  monitor.enter();
  this->exit = PR_TRUE;
  monitor.notifyAll();
  while(this->isAlive) {
    LOG_FINE(( CODELOC, PROTOCOL_HANDLER_LOG_MASK, this->connectionID, MQ_SUCCESS,
              "PingTimer::terminate() waiting for the ping thread to finish .."
             "this->exit=%d", this->exit ));
    monitor.wait();
  }
  monitor.exit();

  LOG_FINE(( CODELOC, PROTOCOL_HANDLER_LOG_MASK, this->connectionID, MQ_SUCCESS,
             "PingTimer::terminate() return;  isAlive=%d "
             "exit=%d ", this->isAlive, this->exit ));
}

