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
 * @(#)mqcallbacks-priv.h	1.10 06/26/07
 */ 

#ifndef MQ_CALLBACKS_PRIV_H
#define MQ_CALLBACKS_PRIV_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mqcallback-types-priv.h"


/**
 * Associates threadCreator with the connection specified by
 * connectionHandle.  Whenever the connection needs to create a
 * thread, it will call threadCreator.  The connection will use it's
 * own method for creating a thread if this method is not called , or
 * NULL is passed for threadCreator.
 *
 * @param connectionHandle handle to the connection to install the callback for
 * @param threadCreator the callback to use for creating threads
 * @param callbackData data to pass to threadCreator */
EXPORTED_SYMBOL MQStatus 
MQSetCreateThreadFunc(MQConnectionHandle        connectionHandle,
                      const MQCreateThreadFunc  threadCreator,
                      void*                     callbackData);

/**
 * Associates messageCallback with the consumer specified by
 * consumerHandle.  Whenever a message arrives for the consumer,
 * messageCallback is invoked.  The thread that invokes this callback
 * is a vital internal thread.  messageCallback should run quickly,
 * and it MUST NOT call any other MQ methods such as
 * MQReceiveMessageNoWait.  Instead, messageCallback should notify
 * another thread that a message has arrived, and this other thread
 * can call MQReceiveMessageNoWait.
 *
 * @param consumerHandle handle to the consumer to install the callback for
 * @param messageCallback the function to call when a message arrives for
 *        the consumer
 * @param callbackData data to pass to messageCallback */
EXPORTED_SYMBOL MQStatus 
MQSetMessageArrivedFunc(const MQConsumerHandle            consumerHandle,
                        MQMessageArrivedFunc              messageCallback,
                        void *                            callbackData);

/**
 * Installs loggingFunc as the callback function to call when messages
 * are logged.
 *
 * @param loggingFunc the function to call when a message is logged
 * @param callbackData data to pass to loggingFunc */
EXPORTED_SYMBOL MQStatus 
MQSetLoggingFunc(const MQLoggingFunc  loggingFunc,
                 void *               callbackData);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MQ_CALLBACKS_PRIV_H */
