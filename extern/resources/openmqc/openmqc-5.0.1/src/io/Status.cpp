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
 * @(#)Status.cpp	1.4 06/26/07
 */ 

#include "Status.hpp"
#include "../util/UtilityMacros.h"

/*
 *
 */
iMQError
Status::toIMQError(const PRInt32 statusCode)
{
  switch(statusCode) {
  case STATUS_OK:                  return IMQ_SUCCESS;
  case STATUS_BAD_REQUEST:         return IMQ_BROKER_BAD_REQUEST ;
  case STATUS_UNAUTHORIZED:        return IMQ_BROKER_UNAUTHORIZED;
  case STATUS_FORBIDDEN:           return IMQ_BROKER_FORBIDDEN;
  case STATUS_NOT_FOUND:           return IMQ_BROKER_NOT_FOUND;
  case STATUS_TIMEOUT:             return IMQ_BROKER_TIMEOUT;
  case STATUS_CONFLICT:            return IMQ_BROKER_CONFLICT;
  case STATUS_GONE:                return IMQ_BROKER_GONE;
  case STATUS_PRECONDITION_FAILED: return IMQ_BROKER_PRECONDITION_FAILED;
  case STATUS_INVALID_LOGIN:       return IMQ_BROKER_INVALID_LOGIN;
  case STATUS_RESOURCE_FULL:       return MQ_BROKER_RESOURCE_FULL;
  case STATUS_ENTITY_TOO_LARGE:    return MQ_BROKER_ENTITY_TOO_LARGE;
  case STATUS_ERROR:               return IMQ_BROKER_ERROR;
  case STATUS_NOT_IMPLEMENTED:     return IMQ_BROKER_NOT_IMPLEMENTED;
  case STATUS_UNAVAILABLE:         return IMQ_BROKER_UNAVAILABLE;
  case STATUS_BAD_VERSION:         return IMQ_BROKER_BAD_VERSION;
  default:                         return IMQ_BROKER_ERROR;
  }
}

