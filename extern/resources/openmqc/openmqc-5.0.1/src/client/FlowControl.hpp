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
 * @(#)FlowControl.hpp	1.3 06/26/07
 */ 

#ifndef FLOWCONTROL_HPP
#define FLOWCONTROL_HPP

#include <nspr.h>
#include "../basictypes/Object.hpp"
#include "../basictypes/Monitor.hpp"

class Connection;

/** Handles flow control with the broker. It keeps track of the number
 *  of packets that have been received but not delivered.  If the
 *  broker pauses the flow of messages, then the FlowControl object
 *  resumes the flow after the number of received, but not delivered
 *  messages falls below the watermark. */
class FlowControl : public Object {
private:

  /** The connection that created this object */
  Connection * connection;

  /** Ensures synchronous access to member variables */
  Monitor monitor;

  /** The number of messages that have been received but not delivered */
  PRInt32 unDeliveredMsgCount;

  /** True iff a resume message has been received, which implies the broker
      is asking to resume sending messages. */
  PRBool resumeRequested;

  /** If the number of undelivered messages is below the water mark,
      it resumes the connection to the broker. */
  void tryResume();

  /** Determines if the connection should be resumed based on the number
      of undelivered messages and the watermark. */
  PRBool shouldResume();

public:
  /** Constructor. */
  FlowControl(Connection * const connection);

  /** Destructor. */
  ~FlowControl();

  /** This method is called when a message is received */
  void messageReceived();

  /** This method is called when a message is delivered */
  void messageDelivered();

  /** This method is called when the broker requests that we resume
      the connection */
  void requestResume();

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  FlowControl(const FlowControl& flowControl);
  FlowControl& operator=(const FlowControl& flowControl);
};

#endif // FLOWCONTROL_HPP
