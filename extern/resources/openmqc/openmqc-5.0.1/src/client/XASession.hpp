/*
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS HEADER.
 *
 * Copyright (c) 2010 Oracle and/or its affiliates. All rights reserved.
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
 * @(#)XASession.hpp	1.2 10/23/07
 */ 

#ifndef XASESSION_HPP
#define XASESSION_HPP

#include "Session.hpp"
#include "../cshim/xaswitch.hpp"

/**
 **/
class XASession : public Session {
private:

  PRUintn xidIndex;
  MQMessageListenerBAFunc beforeMessageListener;
  MQMessageListenerBAFunc afterMessageListener;
  void * baMLCallbackData;
  

public:

  /**
   * @param connection the connection that created this Session
   * @param beforeDeliveryArg for async receiving mode
   * @param afterDeliveryArg  for async receiving mode
   * @paramt callbackDataArg  data pointer to be passed to before/afterDelivery
   * @param receiveMode */
  XASession(Connection *             const connection,
            const ReceiveMode        receiveMode,
            MQMessageListenerBAFunc beforeMessageListenerArg,
            MQMessageListenerBAFunc afterMessageListenerArg,
            void * callbackDataArg);


  MQMessageListenerBAFunc getBeforeMessageListenerFunc();
  MQMessageListenerBAFunc getAfterMessageListenerFunc();
  void * getMessageListenerBACallbackData();
  virtual MQError writeJMSMessage(Message * const message, PRInt64 producerID);
  virtual MQError acknowledge(Message * message, PRBool fromMessageListener);

  /**
   * Destructor.  Calls close, which closes all producers and
   * consumers that were created by this session.  */
  virtual ~XASession();
  
//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  XASession(const XASession& session);
  XASession& operator=(const XASession& session);
};


#endif

