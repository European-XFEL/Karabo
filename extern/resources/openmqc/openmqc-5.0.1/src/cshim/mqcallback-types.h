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
 * @(#)mqcallback-types.h	1.18 11/09/07
 */ 

#ifndef MQ_CALLBACK_TYPES_H
#define MQ_CALLBACK_TYPES_H

/*
 * defines MQ C-API callback types 
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mqtypes.h"

/**
 * This callback is used to notify the user that an exception occurred.
 *
 * @param connectionHandle a handle to the connection on which the
 *        connection exception occurred
 * @param exception the connection exception that occurred
 * @param callbackData whatever void * pointer that was passed to
 *        MQCreateConnection
 * @see MQCreateConnection.  */
typedef void (*MQConnectionExceptionListenerFunc)(
                                const MQConnectionHandle  connectionHandle,
                                MQStatus                  exception,
                                void *                    callbackData );

/**
 * This callback is used for asynchronous receiving messages
 *
 * @param sessionHandle a handle to the session
 * @param consumerHandle a handle to the message consumer 
 * @param messageHandle a handle to the message 
 * @param callbackData whatever void * pointer that was passed to
 *        MQCreateAsyncMessageConsumer or MQCreateAsyncDurableMessageConsumer
 * @see MQCreateAsyncMessageConsumer and MQCreateAsyncDurableMessageConsumer. */
typedef MQError (*MQMessageListenerFunc)(const MQSessionHandle  sessionHandle,
                                         const MQConsumerHandle consumerHandle,
                                         MQMessageHandle        messageHandle,
                                         void *                 callbackData);


/**
 * This callback is called before and after MQMessageListenerFunc call
 * for a XA session. The sessionHandle, consumerHandle, messageHandle
 * are for 'read-only' purpose only.  Please do not call MQFreeMessage  
 * for the messageHandle or MQCloseSession for the sessionHandle or 
 * MQCloseMessageConsumer for the consumerHandle in this callback 
 * function or any fuctions called by this callback. Restrictions in 
 * MQMessageListenerFunc callback also applies to this callback type.
 *
 * @param sessionHandle a handle to the session
 * @param consumerHandle a handle to the message consumer
 * @param messageHandle a handle to the message
 * @param errorCode processing status of the current message
 * @param callbackData whatever void * pointer that was passed to
 *        MQCreateAsyncMessageConsumer or MQCreateAsyncDurableMessageConsumer
 * @see MQCreateAsyncMessageConsumer and MQCreateAsyncDurableMessageConsumer. */
typedef MQError (*MQMessageListenerBAFunc)(const MQSessionHandle  sessionHandle,
                                           const MQConsumerHandle consumerHandle,
                                           const MQMessageHandle  messageHandle,
                                           MQError                errorCode,
                                           void *                 callbackData);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MQ_CALLBACK_TYPES_H */
