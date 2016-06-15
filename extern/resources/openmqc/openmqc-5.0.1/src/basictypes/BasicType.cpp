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
 * @(#)BasicType.cpp	1.5 06/26/07
 */ 

#include "BasicType.hpp"
#include "../util/UtilityMacros.h"

/*
 *
 */
PRBool 
BasicType::getIsBasicType() const
{
  CHECK_OBJECT_VALIDITY();

  return PR_TRUE;
}


/*
 *
 */
iMQError
BasicType::getBoolValue(PRBool * const valueArg) const
{
  RETURN_ERROR_IF_NULL( valueArg );
  *valueArg = PR_FALSE;

  return IMQ_INVALID_TYPE_CONVERSION;
}

/*
 *
 */
iMQError
BasicType::getInt8Value(PRInt8 * const valueArg) const
{
  RETURN_ERROR_IF_NULL( valueArg );
  *valueArg = 0;

  return IMQ_INVALID_TYPE_CONVERSION;
}

/*
 *
 */
iMQError
BasicType::getInt16Value(PRInt16 * const valueArg) const
{
  RETURN_ERROR_IF_NULL( valueArg );
  *valueArg = 0;

  return IMQ_INVALID_TYPE_CONVERSION;
}

/*
 *
 */
iMQError
BasicType::getInt32Value(PRInt32 * const valueArg) const
{
  RETURN_ERROR_IF_NULL( valueArg );
  *valueArg = 0;

  return IMQ_INVALID_TYPE_CONVERSION;
}

/*
 *
 */
iMQError
BasicType::getInt64Value(PRInt64 * const valueArg) const
{
  RETURN_ERROR_IF_NULL( valueArg );
  *valueArg = 0;

  return IMQ_INVALID_TYPE_CONVERSION;
}

/*
 *
 */
iMQError
BasicType::getFloat32Value(PRFloat32 * const valueArg) const
{
  RETURN_ERROR_IF_NULL( valueArg );
  *valueArg = 0.0;

  return IMQ_INVALID_TYPE_CONVERSION;
}

/*
 *
 */
iMQError
BasicType::getFloat64Value(PRFloat64 * const valueArg) const
{
  RETURN_ERROR_IF_NULL( valueArg );
  *valueArg = 0.0;

  return IMQ_INVALID_TYPE_CONVERSION;
}

/*
 *
 */
iMQError
BasicType::getStringValue(const char ** const valueArg) const
{
  RETURN_ERROR_IF_NULL( valueArg );
  *valueArg = "";

  return IMQ_INVALID_TYPE_CONVERSION;
}


