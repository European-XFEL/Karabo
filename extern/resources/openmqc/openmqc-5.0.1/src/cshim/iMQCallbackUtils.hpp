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
 * @(#)iMQCallbackUtils.hpp	1.18 11/09/07
 */ 

#ifndef MQ_CALLBACKUTILS_HPP
#define MQ_CALLBACKUTILS_HPP

#include "mqcallback-types-priv.h"
#include "../client/MessageConsumer.hpp"
#include "../client/Connection.hpp"


MQError invokeMessageListener(const MessageConsumer * consumer,
                              MQMessageListenerFunc   messageListener,
                              void *                  callbackData,
                              Message               * message, PRBool *invoked);

MQError
invokeMessageListenerBA(const MessageConsumer *   consumer,
                        MQMessageListenerBAFunc   messageListenerBA,
                        void *                    callbackData,
                        const Message *           message,
                        MQError                   mqerror, 
                        PRBool *                  invoked);


MQError invokeMessageArrivedCallback(const MessageConsumer * consumer,
                                     MQMessageArrivedFunc callback,
                                     void * callbackData);

MQBool invokeCreateThreadCallback(MQThreadFunc startFunc,
                                   void * arg,
                                   MQCreateThreadFunc callback,
                                   void * callbackData);

void invokeExceptionListenerCallback(const Connection * const connection,
                                     MQError exceptionError,
                                     MQConnectionExceptionListenerFunc callback,
                                     void * callbackData);

void invokeLoggingCallback(const PRInt32 severity,
                           const PRInt32 logCode,
                           ConstMQString logMessage,
                           const PRInt64 timeOfMessage,
                           const PRInt64 connectionID,
                           ConstMQString filename,
                           const PRInt32 fileLineNumber,
                           MQLoggingFunc callback,
                           void* callbackData);

#endif /* MQ_CALLBACKUTILS_HPP */
