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
 * @(#)ProducerFlow.hpp	1.4 06/26/07
 */ 

#ifndef PRODUCERFLOW_HPP
#define PRODUCERFLOW_HPP


#include "Message.hpp"
#include "../basictypes/Object.hpp"
#include "../basictypes/Monitor.hpp"

class Message;

enum FlowState { UNDER_LIMIT, ON_LIMIT, OVER_LIMIT };

/**
 *
 */
class ProducerFlow : public Object {
private:
  PRInt64 producerID;
  Long producerIDLong; //for logging efficiency only 

  //after sending this many bytes the  producer should pause the flow
  PRInt64 chunkBytes;

  //after sending this many messages the producer should pause the flow
  PRInt32 chunkSize; 

  PRInt32 sentCount;
  PRInt32 references;

  PRBool  isClosed;
  MQError closeReason;

  Monitor        monitor;

  FlowState checkFlowLimit();

public:
  ProducerFlow();
  virtual ~ProducerFlow();

  void setProducerID(PRInt64 producerIDArg);
  void setChunkSize(PRInt32 chunkSizeArg);
  void setChunkBytes(PRInt64 chunkBytesArg);

  PRInt64 getProducerID() const;

  MQError checkFlowControl(Message * message);

  void resumeFlow(PRInt64 chunkBytes, PRInt32 chunkSize);

  MQError acquireReference();
  PRBool releaseReference();


  void close(MQError reason);

//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  ProducerFlow(const ProducerFlow& producerFlow);
  ProducerFlow& operator=(const ProducerFlow& producerFlow);
  
};

#endif  // PRODUCERFLOW_HPP
