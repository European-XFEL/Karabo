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
 * @(#)IMQDataOutputStream.cpp	1.3 06/26/07
 */ 

#include "IMQDataOutputStream.hpp"
#include "../util/UtilityMacros.h"
#include "../error/ErrorCodes.h"

/*
 *
 */
iMQError 
IMQDataOutputStream::writeBoolean(const PRBool value)
{
  CHECK_OBJECT_VALIDITY();

  ASSERT( (value == PR_TRUE) || (value == PR_FALSE) );

  // The boolean is only one byte in the file
  PRPackedBool packedBool = (PRPackedBool)value;
  
  return writeUint8((PRUint8)packedBool);
}


/*
 *
 */
iMQError 
IMQDataOutputStream::writeInt8(const PRInt8 value)
{
  CHECK_OBJECT_VALIDITY();

  return writeUint8((PRUint8)value);
}

/*
 *
 */
iMQError 
IMQDataOutputStream::writeUint8Array(const PRUint8 values[], 
                                     const PRUint32 numToWrite)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( values );
  for (PRUint32 i = 0; i < numToWrite; i++) {
    RETURN_IF_ERROR( writeUint8(values[i]) );
  }
  
  return IMQ_SUCCESS;
}

/*
 *
 */
iMQError
IMQDataOutputStream::writeInt16(const PRInt16 value)
{
  CHECK_OBJECT_VALIDITY();

  return writeUint16((PRUint16)value);
}

/*
 *
 */
iMQError
IMQDataOutputStream::writeInt32(const PRInt32 value)
{
  CHECK_OBJECT_VALIDITY();

  return writeUint32((PRUint32)value);
}

/*
 *
 */
iMQError
IMQDataOutputStream::writeInt64(const PRInt64 value)
{
  CHECK_OBJECT_VALIDITY();

  return writeUint64((PRUint64)value);
}

/*
 *
 */
iMQError
IMQDataOutputStream::writeFloat32(const PRFloat32 value)
{
  CHECK_OBJECT_VALIDITY();

  return writeUint32(*((PRUint32*)&value));
}

/*
 *
 */
iMQError
IMQDataOutputStream::writeFloat64(const PRFloat64 value)
{
  CHECK_OBJECT_VALIDITY();

  return writeUint64(*((PRUint64*)&value));
}












