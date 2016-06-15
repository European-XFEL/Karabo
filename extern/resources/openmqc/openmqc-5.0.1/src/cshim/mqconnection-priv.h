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
 * @(#)mqconnection-priv.h	1.13 10/17/07
 */ 

#ifndef MQ_CONNECTION_PRIV_H 
#define MQ_CONNECTION_PRIV_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mqtypes.h"
#include "mqcallback-types-priv.h"
#include "mqproperties.h"
#include "mqconnection-props.h"
#include "mqconnection.h"

/**
 * Opens a new connection the broker based on the supplied parameters.
 *
 * @param propertiesHandle a handle to a properties object containing
 *        properties to be used for this connection.  This handle will
 *        be invalid after this function returns.
 * @param username the username to use when connecting to the broker
 * @param password the password to use when connection to the broker
 * @param clientID the connectionID for the connection
 * @param exceptionListener the connection exception callback function
 * @param exceptionCallbackData the connection exception callback data 
 * @param createThreadFunc the callback function to use when this 
 *        connection creates a thread.  If this parameter is NULL, then
 *        the connection creates its own threads.
 * @param createThreadFuncData the void* function that is passed to
 *        createThreadFunc whenever it is called.
 * @param connectionHandle the output parameter that contains the
 *        newly opened connection.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.
 */
EXPORTED_SYMBOL MQStatus 
MQCreateConnectionExt(MQPropertiesHandle                propertiesHandle,
                      ConstMQString                     username,
                      ConstMQString                     password,
                      ConstMQString                     clientID,
                      MQConnectionExceptionListenerFunc exceptionListener,
                      void *                            listenerCallbackData,
                      MQCreateThreadFunc                createThreadFunc,
                      void *                            createThreadFuncData,
                      MQBool                            isXA,
                      MQConnectionHandle *              connectionHandle);

EXPORTED_SYMBOL MQStatus 
MQCreateXAConnection(MQPropertiesHandle                propertiesHandle,
                     ConstMQString                     username,
                     ConstMQString                     password,
                     ConstMQString                     clientID,
                     MQConnectionExceptionListenerFunc exceptionListener,
                     void *                            listenerCallbackData,
                     MQConnectionHandle *              connectionHandle);
/**
 * 
 * @param connectionHandle the handle to the XA connection to close
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus
MQCloseXAConnection(const MQConnectionHandle connectionHandle);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MQ_CONNECTION_PRIV_H */
