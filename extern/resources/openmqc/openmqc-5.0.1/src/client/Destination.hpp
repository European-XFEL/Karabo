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
 * @(#)Destination.hpp	1.4 06/26/07
 */ 

#ifndef DESTINATION_HPP
#define DESTINATION_HPP


#include "../basictypes/AllBasicTypes.hpp"
#include "../error/ErrorCodes.h"
#include "../basictypes/HandledObject.hpp"

class Connection;

/**
 * This class implements Topic and Queue.  
 */
class Destination : public HandledObject {
protected:
  /** The Connection that created this Destination, used by temporary destination */
  Connection * connection;

  /** The name of the Destination.  */
  UTF8String * name;
  
  /** True iff the destination is a queue */
  PRBool isQueue;

  /** True iff the destination is a temporary */
  PRBool isTemporary;

public:
  /**
   * Constructor.  Stores name.
   */
  Destination(Connection * const connection,
              const UTF8String * const name, 
              const PRBool isQueue, 
              const PRBool isTemporary);

  /**
   * Constructor.
   */
  Destination(const UTF8String * const name, 
              const UTF8String * const className,
              Connection * const connection);

  /**
   * Destructor.  Virtual to allow subclasses' destructors to be called.
   */
  virtual ~Destination();

  /**
   * @return the name of the destination
   */
  virtual const UTF8String * getName() const;

  /**
   * @return the class name for this destination
   */
  virtual const UTF8String * getClassName() const;

  /**
   * @return true if the destination is a queue, and false if it's a topic
   */
  virtual PRBool getIsQueue() const;

  /**
   * @return true iff the destination is temporary
   */
  virtual PRBool getIsTemporary() const;

  /** To implement HandledObject */
  virtual HandledObjectType getObjectType() const;

  /** Deletes this destination from the broker.  This is only valid
      for temporary destinations. */
  iMQError deleteDestination();

  /** @return the Session that created this destination*/
  Connection * getConnection() const;
  
  /** @return a clone of this Destination */
  Destination * clone() const;
  
//
// Avoid all implicit shallow copies.  Without these, the compiler
// will automatically define implementations for us.
//
private:
  //
  // These are not supported and are not implemented
  //
  Destination(const Destination& destination);
  Destination& operator=(const Destination& destination);
};


#endif // DESTINATION_HPP
