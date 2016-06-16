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
 * @(#)iMQLogUtilsShim.cpp	1.12 06/26/07
 */ 

#include "mqlogutil-priv.h"
#include "shimUtils.hpp"
#include "../util/Logger.hpp"


/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQSetLogFileName(ConstMQString logFileName)
{
  static const char FUNCNAME[] = "MQSetLogFileName";
  MQError errorCode = MQ_SUCCESS;
 
  CLEAR_ERROR_TRACE(PR_FALSE);

  Logger * logger = Logger::getInstance();
  CNDCHK( logger == NULL, MQ_STATUS_NULL_LOGGER );

  logger->setLogFileName(logFileName);
  
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQSetLoggingFunc(MQLoggingFunc  loggingFunc,
                   void*    callbackData)
{
  static const char FUNCNAME[] = "MQSetLoggingFunc";
  MQError errorCode = MQ_SUCCESS;

  CLEAR_ERROR_TRACE(PR_FALSE);

  Logger * logger = Logger::getInstance();
  CNDCHK( logger == NULL, MQ_STATUS_NULL_LOGGER );

  ASSERT( sizeof(MQLoggingLevel) == sizeof(LogLevel) );
  logger->setLoggingCallback(loggingFunc, callbackData);
  
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQSetMaxLogSize(MQInt32 maxLogSize)
{
  static const char FUNCNAME[] = "MQSetMaxLogSize";
  MQError errorCode = MQ_SUCCESS;

  CLEAR_ERROR_TRACE(PR_FALSE);

  Logger * logger = Logger::getInstance();
  CNDCHK( logger == NULL, MQ_STATUS_NULL_LOGGER );

  logger->setMaxLogSize(maxLogSize);
  
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQSetLogFileLogLevel(MQLoggingLevel logLevel)
{
  static const char FUNCNAME[] = "MQSetLogFileLogLevel";
  MQError errorCode = MQ_SUCCESS;
  
  CLEAR_ERROR_TRACE(PR_FALSE);

  Logger * logger = Logger::getInstance();
  CNDCHK( logger == NULL, MQ_STATUS_NULL_LOGGER );

  logger->setLogFileLogLevel(logLevel);
  
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQSetStdErrLogLevel(MQLoggingLevel logLevel)
{
  static const char FUNCNAME[] = "MQSetStdErrLogLevel";

  MQError errorCode = MQ_SUCCESS;
  
  CLEAR_ERROR_TRACE(PR_FALSE);

  Logger * logger = Logger::getInstance();
  CNDCHK( logger == NULL, MQ_STATUS_NULL_LOGGER );

  logger->setStdErrLogLevel(logLevel);
  
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQSetCallbackLogLevel(MQLoggingLevel logLevel)
{
  static const char FUNCNAME[] = "MQSetCallbackLogLevel";
  MQError errorCode = MQ_SUCCESS;

  CLEAR_ERROR_TRACE(PR_FALSE);

  Logger * logger = Logger::getInstance();
  CNDCHK( logger == NULL, MQ_STATUS_NULL_LOGGER );

  logger->setCallbackLogLevel(logLevel);
  
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQGetLogFileLogLevel(MQLoggingLevel * logLevel)
{
  static const char FUNCNAME[] = "MQGetLogFileLogLevel";
  MQError errorCode = MQ_SUCCESS;
  Logger * logger = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);

  CNDCHK( logLevel == NULL, MQ_NULL_PTR_ARG );
  
  logger = Logger::getInstance();
  CNDCHK( logger == NULL, MQ_STATUS_NULL_LOGGER );

  *logLevel = (MQLoggingLevel)logger->getLogFileLogLevel();
  
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQGetStdErrLogLevel(MQLoggingLevel * logLevel)
{
  static const char FUNCNAME[] = "MQGetStdErrLogLevel";
  MQError errorCode = MQ_SUCCESS;
  Logger * logger = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);
  
  CNDCHK( logLevel == NULL, MQ_NULL_PTR_ARG );
  
  logger = Logger::getInstance();
  CNDCHK( logger == NULL, MQ_STATUS_NULL_LOGGER );

  *logLevel = (MQLoggingLevel)logger->getStdErrLogLevel();
  
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQGetCallbackLogLevel(MQLoggingLevel *  logLevel)
{
  static const char FUNCNAME[] = "MQGetCallbackLogLevel";
  MQError errorCode = MQ_SUCCESS;
  Logger * logger = NULL;
  
  CLEAR_ERROR_TRACE(PR_FALSE);

  CNDCHK( logLevel == NULL, MQ_NULL_PTR_ARG );

  logger = Logger::getInstance();
  CNDCHK( logger == NULL, MQ_STATUS_NULL_LOGGER );

  *logLevel = (MQLoggingLevel)logger->getCallbackLogLevel();
  
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQSetLogMask(MQLoggingLevel logLevel, MQInt32 logMask)
{
  static const char FUNCNAME[] = "MQSetLogMask";
  MQError errorCode = MQ_SUCCESS;

  CLEAR_ERROR_TRACE(PR_FALSE);

  Logger * logger = Logger::getInstance();
  CNDCHK( logger == NULL, MQ_STATUS_NULL_LOGGER );

  logger->setMask(logLevel, logMask);
  
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}

/*
 *
 */
EXPORTED_SYMBOL MQStatus
MQGetLogMask(MQLoggingLevel logLevel, MQInt32 * logMask)
{
  static const char FUNCNAME[] = "MQGetLogMask";
  MQError errorCode = MQ_SUCCESS;
  Logger * logger = NULL;

  CLEAR_ERROR_TRACE(PR_FALSE);

  CNDCHK( logMask == NULL, MQ_NULL_PTR_ARG );
  
  logger = Logger::getInstance();
  CNDCHK( logger == NULL, MQ_STATUS_NULL_LOGGER );

  *logMask = logger->getMask(logLevel);
  
  RETURN_STATUS( MQ_SUCCESS );
Cleanup:
  MQ_ERROR_TRACE( FUNCNAME, errorCode );
  RETURN_STATUS( errorCode );
}
