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
 * @(#)StubProtocolHandler.cpp	1.3 06/26/07
 */ 

#include "StubProtocolHandler.hpp"
#include "../../util/UtilityMacros.h"
#include "../../error/ErrorCodes.h"

 
StubProtocolHandler::StubProtocolHandler(const char * fileToReadArg, 
                                         const char * fileToWriteArg)
{
  CHECK_OBJECT_VALIDITY();

  if ((fileToReadArg == NULL) || (fileToWriteArg ==NULL)) {
    return;
  }

  FILE * in = fopen(fileToReadArg, "rb");
  if (in == NULL) {
    return;
  }

  bufferSize = (PRUint32)fread(buffer, sizeof(PRUint8), sizeof(buffer), in);
  fclose(in);
  inputStream.setNetOrderStream(buffer, bufferSize);

  this->fileToWrite = fileToWriteArg;
}

StubProtocolHandler::~StubProtocolHandler()
{
  CHECK_OBJECT_VALIDITY();

  FILE* out = fopen(fileToWrite, "wb");
  outputStream.writeToFile(out);
  fclose(out);
}



iMQError 
StubProtocolHandler::connect(const Properties * const connectionProperties)
{
  CHECK_OBJECT_VALIDITY();

  UNUSED( connectionProperties );
  return IMQ_SUCCESS;
}


iMQError 
StubProtocolHandler::getLocalPort(PRUint16 * const port) const
{
  CHECK_OBJECT_VALIDITY();

  *port = 0;
  return IMQ_SUCCESS;
}

iMQError
StubProtocolHandler::read(const PRInt32          numBytesToRead,
                          const PRUint32         timeoutMicroSeconds, 
                                PRUint8 * const  bytesRead, 
                                PRInt32 * const  numBytesRead)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_IF_ERROR( inputStream.readUint8Array(bytesRead, numBytesToRead) );
  UNUSED( timeoutMicroSeconds );

  *numBytesRead = numBytesToRead;

  return IMQ_SUCCESS;
}

iMQError
StubProtocolHandler::write(const PRInt32          numBytesToWrite,
                           const PRUint8 * const  bytesToWrite,
                           const PRUint32         timeoutMicroSeconds, 
                                 PRInt32 * const  numBytesWritten)
{
  CHECK_OBJECT_VALIDITY();

  UNUSED( timeoutMicroSeconds );

  outputStream.writeUint8Array(bytesToWrite, numBytesToWrite);
  *numBytesWritten = numBytesToWrite;

  return IMQ_SUCCESS;
}


iMQError 
StubProtocolHandler::close()
{
  CHECK_OBJECT_VALIDITY();

  return IMQ_SUCCESS;
}

iMQError 
StubProtocolHandler::shutdown()
{
  CHECK_OBJECT_VALIDITY();

  return IMQ_SUCCESS;
}

PRBool 
StubProtocolHandler::isClosed()
{
  CHECK_OBJECT_VALIDITY();

  return PR_FALSE;
}

iMQError 
StubProtocolHandler::getLocalIP(const IPAddress ** const ipAddr) const
{
  CHECK_OBJECT_VALIDITY();

  *ipAddr = NULL;
  return IMQ_SUCCESS;
}
