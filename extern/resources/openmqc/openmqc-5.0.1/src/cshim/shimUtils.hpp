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
 * @(#)shimUtils.hpp	1.8 06/26/07
 */ 

#ifndef SHIMUTILS_HPP
#define SHIMUTILS_HPP


#include "mqtypes.h"
#include "../util/UtilityMacros.h"
#include "../basictypes/HandledObject.hpp"


/**
 * Return a pointer to an object, with the given handle.  This
 * acquires an external reference to the object, which must be
 * released by calling releaseHandledObject.
 *
 * @param handle a handle to the object to retrieve.
 * @param objectType the expected type of the object.  See
 *        HandledObject.hpp for a list of valid types.
 * @return a pointer to the handled object.  If the handle is
 *         invalid or the type of the handled object does not
 *         match objectType, then NULL is returned.  */
HandledObject *
getHandledObject(const MQObjectHandle handle,
                 const HandledObjectType objectType);

/**
 * Releases an external reference to object.  This might actually
 * delete object.  This will occur if object was deleted internally by
 * the library (e.g. due to the connection being closed) but could not
 * actually be deleted because an external reference was held.
 *
 * @param object the object to release the handle to.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
MQStatus
releaseHandledObject(HandledObject * object);

/**
 * Deletes the object with the given handle.
 *
 * @param handle the handle of the object to delete.
 * @param objectType the type of the object.  See
 *        HandledObject.hpp for a list of valid types.
 * @return the status of the function call.  Pass this value to
 *         MQStatusIsError to determine if the call was
 *         successful.  */
MQStatus
freeHandledObject(const MQObjectHandle handle, 
                  const HandledObjectType objectType);


/**
 * This macro returns an MQStatus struct with the .errorCode field
 * set to error.  */
#define RETURN_STATUS(error)                \
  IMQ_BEGIN_MACRO                           \
    MQStatus i_M_Q_S_t_a_t_u_s;            \
    i_M_Q_S_t_a_t_u_s.errorCode = (error);  \
    return i_M_Q_S_t_a_t_u_s;               \
  IMQ_END_MACRO


/**
 * If expr is true, this macro returns an MQStatus struct with the
 * .errorCode field set to error.  Otherwise it does nothing.  
 */
#define RETURN_STATUS_IF(expr,error)          \
  IMQ_BEGIN_MACRO                             \
    if (expr) {                               \
      MQStatus i_M_Q_S_t_a_t_u_s;            \
      i_M_Q_S_t_a_t_u_s.errorCode = (error);  \
      return i_M_Q_S_t_a_t_u_s;               \
    }                                         \
  IMQ_END_MACRO


#endif /* SHIMUTILS_HPP */



