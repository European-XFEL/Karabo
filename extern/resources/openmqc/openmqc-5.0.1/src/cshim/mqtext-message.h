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
 * @(#)mqtext-message.h	1.10 06/26/07
 */ 

#ifndef MQ_TEXT_MESSAGE_H
#define MQ_TEXT_MESSAGE_H

/*
 * declarations of C interface for text message
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mqtypes.h"

/**
 * Creates a new text message.
 *
 * @param messageHandle the output parameter for the newly created message
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */   
EXPORTED_SYMBOL MQStatus 
MQCreateTextMessage(MQMessageHandle * messageHandle);
  
/**
 * Gets the bytes from a bytes message.  This call is only valid if
 * messageHandle refers to a message whose type is MQTextMessageType.
 * The string that is returned is not a copy.  The caller should not
 * modify the bytes or attempt to free it.  The string is a NULL-
 * terminated UTF8-encoded string.
 *
 * @param messageHandle the handle of the message to retrieve the text from
 * @param messageText the output parameter for the message text
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */   
EXPORTED_SYMBOL MQStatus 
MQGetTextMessageText(const MQMessageHandle messageHandle,
                     ConstMQString *       messageText);

/**
 * Sets the text for a text message.  This call is only valid if
 * messageHandle refers to a message whose type is
 * MQTextMessageType.  A copy of the string is made, so the caller
 * can manipulate messageText after this function returns.  messageText
 * should be a NULL-terminated UTF8-encoded string.
 *
 * @param messageHandle the handle of the message to set the text 
 * @param messageText the string to set the message text to
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */   
EXPORTED_SYMBOL MQStatus 
MQSetTextMessageText(const MQMessageHandle messageHandle,
                     ConstMQString         messageText);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MQ_TEXT_MESSAGE_H */
