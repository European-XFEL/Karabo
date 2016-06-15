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
 * @(#)mqlogutil-priv.h	1.13 06/26/07
 */ 

#ifndef MQ_LOGUTIL_PRIV_H
#define MQ_LOGUTIL_PRIV_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mqtypes.h"  
#include "mqcallback-types-priv.h"
#include "mqcallbacks-priv.h"
  

/**
 * Sets the log file name.  If no file name is set, then the logging
 * will go to stderr (default).  If log file name is set, the log file
 * names will be <logFileName>.N, where N is 0, 1, 2 ... if no '%g'
 * pattern in the part of logFileName after the last directory separator;
 * otherwise the log file names will be a set of logFileName with the 
 * last '%g' substituted by 0, 1, 2 ....  Use '%%g' to escape the last '%g'
 * substitution. The latest log file is the one with 0 index.  If logFileName
 * is the name of a directory, it should include a trailing directory separator.
 *
 * This function is not MT safe
 *
 * @param logFileName the base file name to use for the log file names.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQSetLogFileName(ConstMQString logFileName);

/**
 * Sets the callback function to invoke whenever information is
 * logged.
 *
 * @param loggingFunc the callback function
 * @param callbackData data to be passed to the callback function
 *        when it is called 
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQSetLoggingFunc(MQLoggingFunc  loggingFunc,
                 void *         callbackData);

/**
 * Sets the maximum bytes to write a log file.  The actual log size
 * will slightly exceed this because the log is closed whenever its
 * size exceeds the maximum size.
 *
 * @param maxLogSize the maximum size of the log in bytes
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQSetMaxLogSize(MQInt32 maxLogSize);

/**
 * Sets the minimum logging level at which log messages should be
 * logged to the log file.  All messages with a logging level of at
 * least logLevel will be logged, and all messages with a logging
 * level less than logLevel will not be logged.  For example, if
 * logLevel is MQ_LOG_WARNING, then only messages with a log level
 * of MQ_LOG_WARNING and MQ_LOG_SEVERE will be logged.
 *
 * @param logLevel the minimum logging level at which log messages
 *        are logged to the log file.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQSetLogFileLogLevel(MQLoggingLevel logLevel);

/**
 * Sets the minimum logging level at which log messages should be
 * logged to stderr.  All messages with a logging level of at least
 * logLevel will be logged, and all messages with a logging level less
 * than logLevel will not be logged.
 *
 * @param logLevel the minimum logging level at which log messages
 *         are logged to stderr.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQSetStdErrLogLevel(MQLoggingLevel logLevel);

/**
 * Sets the minimum logging level at which log messages should be
 * passed to the application installed logging callback function.  All
 * messages with a logging level of at least logLevel will be logged,
 * and all messages with a logging level less than logLevel will not
 * be logged.
 *
 * @param logLevel the minimum logging level at which log messages
 *        are passed to the logging callback.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQSetCallbackLogLevel(MQLoggingLevel logLevel);
  
/**
 * Gets the minimum logging level at which log messages should be
 * logged to the log file.
 *
 * @param logLevel output parameter for the minimum logging level
 *        at which log messages are logged to the log file.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQGetLogFileLogLevel(MQLoggingLevel * logLevel);

/**
 * Gets the minimum logging level at which log messages should be
 * logged to stderr.
 *
 * @param logLevel output parameter for the minimum logging level
 *        at which log messages are logged to stderr
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQGetStdErrLogLevel(MQLoggingLevel * logLevel);

/**
 * Gets the minimum logging level at which log messages should be
 * passed to the application installed logging callback.
 *
 * @param logLevel output parameter for the minimum logging level
 *        at which log messages are passed to the application installed
 *        logging callback.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQGetCallbackLogLevel(MQLoggingLevel * logLevel);


/**
 * The following two methods won't mean anything to someone who
 * doesn't have access to the MQ C Client source code (or at least
 * LogUtils.hpp).  They are provided primarily to reduce the number of
 * logging messages when debugging a specific component of the C
 * client. */


/**
 * Sets the log mask for logLevel to logMask.  This controls what
 * messages are actually logged.
 * 
 * @param logLevel specifies the logging level to which logMask applies.
 * @param logMask the logging mask to use to filter out logging messages.
 *        The component mask of the logging message will be bitwise AND'ed with
 *        the logMask, and only if the result is nonzero will it be logged.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQSetLogMask(MQLoggingLevel logLevel, MQInt32 logMask);

/**
 * Gets the log mask for logLevel to logMask.  This controls what
 * messages are actually logged.
 * 
 * @param logLevel specifies the logging level to which logMask applies.
 * @param logMask the output parameter for the current logging mask
 *        that is used to filter out logging messages.  The component
 *        mask of the logging message will be bitwise AND'ed with the
 *        logMask, and only if the result is nonzero will it be
 *        logged.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQGetLogMask(MQLoggingLevel logLevel, MQInt32 * logMask);

  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MQ_LOGUTIL_PRIV_H */
