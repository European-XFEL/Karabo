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
 * @(#)mqmessage.h	1.15 06/26/07
 */ 

#ifndef MQ_MESSAGE_H
#define MQ_MESSAGE_H

/*
 * declarations of C interface for message
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mqtypes.h"

#include "mqheader-props.h"


/**
 * Creates a new MQ_MESSAGE type message.
 *
 * @param messageHandle the output parameter for the newly created message
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus
MQCreateMessage(MQMessageHandle * messageHandle);

  
/**
 * Frees the message specified by messageHandle.
 *
 * @param messageHandle the message to free.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQFreeMessage(MQMessageHandle messageHandle);

/**
 * Returns the type of the message.
 *
 * @param messageHandle the message to return the type for
 * @param messageType the output parameter for the type of the message
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQGetMessageType(const MQMessageHandle messageHandle,
                 MQMessageType *       messageType);
  
/**
 * Returns the properties of the message.  The caller is responsible
 * for freeing the returned properties by calling MQFreeProperties.
 * Message properties are optional and application specific.
 *
 * @param messageHandle the message to return the properties for
 * @param propertiesHandle the output parameter for the properties
 *        of this message
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQGetMessageProperties(const MQMessageHandle messageHandle,
                       MQPropertiesHandle *  propertiesHandle);

/**
 * Sets the properties of the message.  This function replaces all
 * message properties with the properties specified in
 * propertiesHandle.  propertiesHandle will be invalid after this call
 * returns.
 *
 * @param messageHandle the message to set the properties for
 * @param propertiesHandle the properties to set for this message
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQSetMessageProperties(const MQMessageHandle    messageHandle,
                       MQPropertiesHandle propertiesHandle);

/**
 * Returns the headers of the message.  The caller is responsible
 * for freeing the returned headers by calling MQFreeProperties.
 *
 * @param messageHandle the message to return the properties for
 * @param headersHandle the output parameter for the headers
 *        of this message
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQGetMessageHeaders(const MQMessageHandle messageHandle,
                    MQPropertiesHandle *  headersHandle);

/**
 * Sets the headers of the message.  Headers fields not specified in
 * messageHandle are not affected -- only the header fields specified
 * by headersHandle are changed.  headersHandle will be invalid after
 * this call returns.
 *
 * @param messageHandle the message to set the headers for
 * @param headersHandle the headers to set for this message
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQSetMessageHeaders(const MQMessageHandle    messageHandle,
                    MQPropertiesHandle headersHandle);
  
/**
 * Sets the reply to destination for this message.  destinationHandle
 * is still valid after calling this function.
 *
 * @param messageHandle the message to set the reply to destination for
 * @param destinationHandle the reply to destination for this message
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQSetMessageReplyTo(const MQMessageHandle messageHandle,
                      const MQDestinationHandle destinationHandle);

/**
 * Gets the reply to destination for this message.  The caller is
 * responsible for freeing destinationHandle by calling
 * MQFreeDestination.
 *
 * @param messageHandle the message to set the reply to destination for
 * @param destinationHandle the output parameter for the reply to destination
 *        of this message
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQGetMessageReplyTo(const MQMessageHandle messageHandle,
                    MQDestinationHandle * destinationHandle);
  
/**
 * Acknowledges the message specified by messageHandle and all other
 * messages that were received before it on the same session.
 *
 * @param sessionHandle the session on which the message was delivered to 
 * @param messageHandle the message handle to the message that is to be
 *        acknowledged
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
EXPORTED_SYMBOL MQStatus 
MQAcknowledgeMessages(const MQSessionHandle sessionHandle,
                      const MQMessageHandle  messageHandle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MQ_MESSAGE_H */
