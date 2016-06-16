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
 * @(#)IMQDataInputStream.cpp	1.3 06/26/07
 */ 

#include "../debug/DebugUtils.h"
#include "IMQDataInputStream.hpp"
#include "../util/UtilityMacros.h"

/*
 * 
 */
iMQError 
IMQDataInputStream::readBoolean(PRBool * const value)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( value );
  *value = PR_FALSE;

  // The boolean is only one byte in the file
  PRPackedBool packedBool = PR_FALSE;
  RETURN_IF_ERROR( readUint8((PRUint8*)&packedBool) );

  *value = packedBool;

  return IMQ_SUCCESS;
}


/*
 *
 */
iMQError 
IMQDataInputStream::readUint8Array(PRUint8 * const values, 
                                   const PRUint32 numToRead)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( values );

  for (PRUint32 i = 0; i < numToRead; i++) {
    RETURN_IF_ERROR( readUint8(&(values[i])) );
  }

  return IMQ_SUCCESS;
}

/*
 *
 */
iMQError
IMQDataInputStream::readInt8(PRInt8 * const value)
{
  CHECK_OBJECT_VALIDITY();

  return readUint8((PRUint8*)value);
}

/*
 *
 */
iMQError
IMQDataInputStream::readInt16(PRInt16 * const value)
{
  CHECK_OBJECT_VALIDITY();

  return readUint16((PRUint16*)value);
}

/*
 *
 */
iMQError
IMQDataInputStream::readInt32(PRInt32 * const value)
{
  CHECK_OBJECT_VALIDITY();

  return readUint32((PRUint32*)value);
}

/*
 *
 */
iMQError 
IMQDataInputStream::readInt64(PRInt64 * const value)
{
  CHECK_OBJECT_VALIDITY();

  return readUint64((PRUint64*)value);
}

/*
 *
 */
iMQError 
IMQDataInputStream::readFloat32(PRFloat32 * const value)
{
  CHECK_OBJECT_VALIDITY();

  return readUint32((PRUint32*)value);
}

/*
 *
 */
iMQError 
IMQDataInputStream::readFloat64(PRFloat64 * const value)
{
  CHECK_OBJECT_VALIDITY();

  return readUint64((PRUint64*)value);
}
