/*
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS HEADER.
 *
 * Copyright (c) 2000-2013 Oracle and/or its affiliates. All rights reserved.
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
 * @(#)SSLProtocolHandler.cpp	1.7 06/26/07
 */ 

#include "SSLProtocolHandler.hpp"
#include "../../util/UtilityMacros.h"
#include "../../util/LogUtils.hpp"
#include "../PortMapperClient.hpp"

/*
 *
 */
SSLProtocolHandler::SSLProtocolHandler()
{
  CHECK_OBJECT_VALIDITY();

  init();
}



/*
 *
 */
SSLProtocolHandler::~SSLProtocolHandler()
{
  CHECK_OBJECT_VALIDITY();

  reset();
}

/*
 *
 */
void
SSLProtocolHandler::init()
{
  CHECK_OBJECT_VALIDITY();

}

/*
 *
 */
void
SSLProtocolHandler::reset()
{
  CHECK_OBJECT_VALIDITY();

  brokerSocket.reset();
  this->init();
}


/*
 *
 */
MQError 
SSLProtocolHandler::connect(const Properties * const connectionProperties)
{
  CHECK_OBJECT_VALIDITY();

  MQError errorCode = MQ_SUCCESS;

  PortMapperClient portMapperClient;
  UTF8String sslProtocol(SSL_PROTOCOL_STR);
  UTF8String normalConnection(CONNECTION_TYPE_NORMAL_STR);
  PRInt32 directPort = 0;
  PRUint16 brokerPort = 0;
  const char * brokerName = NULL;
  PRBool useIPV6 = PR_FALSE;
  PRUint32 connectTimeout = 0;
  PRBool brokerIsTrusted;
  PRBool checkBrokerFingerprint;
  const char * brokerCertFingerprint = NULL;

  // Make sure we are not already connected, and that
  // connectionProperties is valid
  CNDCHK( brokerSocket.status() != TCPSocket::NOT_CONNECTED, 
          MQ_TCP_ALREADY_CONNECTED );
  NULLCHK( connectionProperties );

  // Get the host name property first
  ERRCHK( connectionProperties->getStringProperty(MQ_BROKER_HOST_PROPERTY, 
                                                  &brokerName) );
  NULLCHK(brokerName);
  if ( connectionProperties->getIntegerProperty(MQ_SERVICE_PORT_PROPERTY,
                                                &directPort) == MQ_SUCCESS) {
    CNDCHK( (directPort <= 0 || 
             directPort > PORT_MAPPER_CLIENT_MAX_PORT_NUMBER), MQ_TCP_INVALID_PORT );
    brokerPort = directPort; 
  }
  errorCode = connectionProperties->getBooleanProperty(
                MQ_ENABLE_IPV6_PROPERTY, &useIPV6);
  if (errorCode != MQ_SUCCESS && errorCode != MQ_NOT_FOUND) {
      ERRCHK( errorCode );
  }
  if (errorCode == MQ_NOT_FOUND) {
    useIPV6 = MQ_FALSE;
  }

  if (brokerPort == 0) {
    // Use the portmapper to find out what port to connect to
    ERRCHK( portMapperClient.readBrokerPorts(connectionProperties) );
    ERRCHK( portMapperClient.getPortForProtocol(&sslProtocol,
                                                &normalConnection,
                                                &brokerPort) );
  } 

  // Get SSL connection properties
  if (connectionProperties->getBooleanProperty(MQ_SSL_BROKER_IS_TRUSTED,
                                              &brokerIsTrusted))
  {
    brokerIsTrusted = DEFAULT_SSL_BROKER_IS_TRUSTED;
  }
  if (connectionProperties->getBooleanProperty(MQ_SSL_CHECK_BROKER_FINGERPRINT,
                                               &checkBrokerFingerprint))
  {
    checkBrokerFingerprint = DEFAULT_SSL_CHECK_BROKER_FINGERPRINT;
  }
  if (connectionProperties->getStringProperty(MQ_SSL_BROKER_CERT_FINGERPRINT,
                                              &brokerCertFingerprint))
  {
    brokerCertFingerprint = DEFAULT_SSL_HOST_CERT_FINGERPRINT;
  }

  // Set SSL socket properties
  ERRCHK( this->brokerSocket.setSSLParameters(brokerIsTrusted,
                                              checkBrokerFingerprint,
                                              brokerCertFingerprint) );
  
  // We should probably get this from a property
  connectTimeout = DEFAULT_CONNECT_TIMEOUT;

  // Now connect to the broker on the JMS port
  ERRCHK( this->brokerSocket.connect(brokerName, 
                                     brokerPort, useIPV6,
                                     connectTimeout) );

  LOG_INFO(( CODELOC, SSL_HANDLER_LOG_MASK, NULL_CONN_ID, MQ_SUCCESS,
             "Opened SSL connection to broker %s:%d.", brokerName, brokerPort ));
  
  return MQ_SUCCESS;

Cleanup:
  LOG_SEVERE(( CODELOC, SSL_HANDLER_LOG_MASK, NULL_CONN_ID, 
               MQ_COULD_NOT_CONNECT_TO_BROKER,
               "Could not open SSL connection to broker %s:%d because '%s' (%d)", 
               (brokerName == NULL ? "NULL":brokerName), 
               brokerPort, errorStr(errorCode), errorCode ));
  
  return errorCode;
}


/*
 *
 */
MQError 
SSLProtocolHandler::getLocalPort(PRUint16 * const port) const
{
  CHECK_OBJECT_VALIDITY();

  return brokerSocket.getLocalPort(port);
}

/*
 *
 */
MQError 
SSLProtocolHandler::getLocalIP(const IPAddress ** const ipAddr) const
{
  CHECK_OBJECT_VALIDITY();

  return brokerSocket.getLocalIP(ipAddr);
}


/*
 *
 */
MQError 
SSLProtocolHandler::read(const PRInt32          numBytesToRead,
                         const PRUint32         timeoutMicroSeconds, 
                               PRUint8 * const  bytesRead, 
                               PRInt32 * const  numBytesRead)
{
  CHECK_OBJECT_VALIDITY();

  return brokerSocket.read(numBytesToRead,
                           timeoutMicroSeconds,
                           bytesRead,
                           numBytesRead);
}


/*
 *
 */
MQError 
SSLProtocolHandler::write(const PRInt32          numBytesToWrite,
                          const PRUint8 * const  bytesToWrite,
                          const PRUint32         timeoutMicroSeconds, 
                                PRInt32 * const  numBytesWritten)
{
  CHECK_OBJECT_VALIDITY();

  return brokerSocket.write(numBytesToWrite,
                            bytesToWrite,
                            timeoutMicroSeconds,
                            numBytesWritten);
}

/*
 *
 */
MQError 
SSLProtocolHandler::close()
{
  CHECK_OBJECT_VALIDITY();

  return brokerSocket.close();
}


/*
 *
 */
MQError 
SSLProtocolHandler::shutdown()
{
  CHECK_OBJECT_VALIDITY();

  return brokerSocket.shutdown();
}

/*
 *
 */
PRBool
SSLProtocolHandler::isClosed()
{
  CHECK_OBJECT_VALIDITY();

  return brokerSocket.isClosed();
}

/*
 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib
 odbccp32.lib wsock32.lib winmm.lib libnspr4.lib libplds4.lib
 libplc4.lib certdb.lib certhi.lib crmf.lib crypto.lib cryptohi.lib
 fort.lib fort32.lib freebl.lib jar.lib nss.lib nssb.lib nssckbi.lib
 nssckfw.lib pk11wrap.lib pkcs12.lib pkcs7.lib sectool.lib secutil.lib
 smime.lib dbm.lib softoken.lib ssl.lib swfci.lib swft.lib swft32.lib
 zlib.lib */

// kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnspr4.lib libplds4.lib libplc4.lib sectool.lib smime.lib ssl.lib nss.lib wsock32.lib winmm.lib

/*
certdb.lib certhi.lib crmf.lib crypto.lib cryptohi.lib fort.lib
fort32.lib freebl.lib jar.lib nss.lib nssb.lib nssckbi.lib nssckfw.lib
pk11wrap.lib pkcs12.lib pkcs7.lib sectool.lib secutil.lib smime.lib
dbm.lib softoken.lib ssl.lib swfci.lib swft.lib swft32.lib zlib.lib */


/*
cl WINNT5.0_DBG.OBJD\\client.obj -FeWINNT5.0_DBG.OBJD/client.exe -link
-DEBUG -DEBUGTYPE:CV
..\\..\\..\\..\\dist\\WINNT5.0_DBG.OBJD\\lib\\sectool.lib
..\\..\\..\\..\\dist\\WINNT5.0_DBG.OBJD\\lib\\smime3.lib
..\\..\\..\\..\\dist\\WINNT5.0_DBG.OBJD\\lib\\ssl3.lib
..\\..\\..\\..\\dist\\WINNT5.0_DBG.OBJD\\lib\\nss3.lib
..\\..\\..\\..\\dist\\WINNT5.0_DBG.OBJD\\lib\\libplc4.lib
..\\..\\..\\..\\dist\\WINNT5.0_DBG.OBJD\\lib\\libplds4.lib
..\\..\\..\\..\\dist\\WINNT5.0_DBG.OBJD\\lib\\libnspr4.lib
wsock32.lib winmm.lib **/



// kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib winmm.lib libnspr4.lib libplds4.lib libplc4.lib certdb.lib certhi.lib crmf.lib crypto.lib cryptohi.lib fort.lib fort32.lib freebl.lib jar.lib nss.lib nssb.lib nssckbi.lib nssckfw.lib pk11wrap.lib pkcs12.lib pkcs7.lib sectool.lib secutil.lib smime.lib dbm.lib softoken.lib ssl.lib swfci.lib swft.lib swft32.lib zlib.lib

// wsock32.lib winmm.lib

// kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnspr4.lib libplds4.lib libplc4.lib sectool.lib smime3.lib ssl3.lib nss3.lib



//certdb.lib certhi.lib crmf.lib crypto.lib cryptohi.lib fort.lib fort32.lib freebl.lib jar.lib nss.lib nssb.lib nssckbi.lib nssckfw.lib pk11wrap.lib pkcs12.lib pkcs7.lib sectool.lib secutil.lib smime.lib dbm.lib softoken.lib ssl.lib swfci.lib swft.lib swft32.lib zlib.lib


//C:\Program Files\Common Files\WebGain Shared;C:\tools\WebGain\VCafe\Jdk13\Bin;C:\tools\WebGain\VCafe\Bin;C:\tools\WebGain\BEA\wlserver6.1sp1\bin;c:\bin\utils\doxygen-1.2.11.1\bin;C:\apps\emacs-20.7\bin;C:\ely\imq\imqcclient\lib\nss\v3.3.1\WINNT4.0_DBG.OBJ\lib;c:\ely\imq\imqcclient\lib\nspr\WINNT4.0_DBG.OBJ\lib;

//C:\Perl\bin\;%SystemRoot%\system32;%SystemRoot%;%SystemRoot%\System32\Wbem;C:\imq\bin;C:\PROGRA~1\ULTRAE~1;C:\WebSphere\AppServer\bin;c:\jdk1.3.1_01\bin;c:\Program Files\Microsoft Visual Studio .NET\vc7\bin;C:\Program Files\GNU\WinCvs 1.2;c:\ely\nss\moz_tools\bin;c:\bin\cygwin\bin;c:\bin\bat;c:\Program Files\Microsoft SQL Server\80\Tools\Binn\;C:\MSSQL7\BINN
