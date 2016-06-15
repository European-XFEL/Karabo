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

#include <stdio.h>
#include <string.h> 
#include <atmi.h>      /* TUXEDO */
#include <userlog.h>   /* TUXEDO */
#include <mqcrt.h> 
#include <unistd.h>    /* please change this for Windows */

/***********************************************************************
 *                                                                     
 * Please configure the service to run in its own Tuxedo server instance
 *
 * To run, 
 *
 * 1. start a broker instance
 * 2. configure Tuxedo server(s) for #4 and #6  
 * 3. tmboot
 * 4. run jmsclient_sender example 
 * 5. run imqcmd list dst to see messages in the destination
 * 6. run jmsclient_async_receiver
 * 7. run imqcmd list dst to see messages in the destination has consumed
 ************************************************************************/


#define MQ_ERR_CHK(mqCall)                             \
  if (MQStatusIsError(status = (mqCall)) == MQ_TRUE) { \
    goto Cleanup;                                      \
  }


MQError 
beforeMessageDelivery(const MQSessionHandle sessionHandle,
                      const MQConsumerHandle consumerHandle,
                      MQMessageHandle message,
                      MQError mqerror,
                      void * callbackData);
MQError
afterMessageDelivery(const MQSessionHandle sessionHandle,
                     const MQConsumerHandle consumerHandle,
                     MQMessageHandle message,
                     MQError mqerror,
                     void * callbackData);

MQError 
messageListenerFunc(const MQSessionHandle sessionHandle,
                    const MQConsumerHandle consumerHandle,
                    MQMessageHandle message,
                    void * callbackData);

/*
 * Asynchronous message receiving service should run in its
 * own dedicated server.
 *
 *
 * A Tuxedo client calls this in non-transaction context.
 *
 */


static MQBool g_done = MQ_FALSE;
static MQBool g_err = MQ_FALSE;

void
ARECVMESSAGES(TPSVCINFO *rqst)
{
    MQSessionHandle session = MQ_INVALID_HANDLE;
    MQConnectionHandle connection = MQ_INVALID_HANDLE;
    MQDestinationHandle queue = MQ_INVALID_HANDLE;
    MQConsumerHandle consumer = MQ_INVALID_HANDLE;
    MQMessageHandle message = MQ_INVALID_HANDLE;
    MQStatus status;

    TPCONTEXT_T context;

    printf("async_jmsserver: ARECVMESSAGES started\n");

    if (tpgetctxt(&context, 0) < 0) { 
        printf("async_jmsserver: tpgetctxt() failed: tperrno=%d\n", tperrno);
	    tpreturn(TPFAIL, -1, NULL, 0L, 0);
    }

    /* Get XA Connection */

    MQ_ERR_CHK( MQGetXAConnection(&connection) );

    /* Stop the connection */

    MQ_ERR_CHK( MQStopConnection(connection) );


    /* Create a XA Session */

    MQ_ERR_CHK( MQCreateXASession(connection, MQ_SESSION_ASYNC_RECEIVE, 
                               &beforeMessageDelivery, 
                               &afterMessageDelivery, 
                               &context, &session) );

    /* Create the destination */

    MQ_ERR_CHK( MQCreateDestination(session, "xatestqueue",
                                    MQ_QUEUE_DESTINATION, &queue) );

    /* Create the async message consumer */

    MQ_ERR_CHK( MQCreateAsyncMessageConsumer(session, queue, NULL, MQ_TRUE,
                                             &messageListenerFunc, NULL, &consumer) );

    /* Free the destination handle */

    MQ_ERR_CHK( MQFreeDestination(queue) );


    /* Start the connection */

    MQ_ERR_CHK( MQStartConnection(connection) );

    printf("async_jmsserver: Wait for message consuming done ...\n");

    while (1) {

        sleep(1);     //please change this for Windows

        if (g_done == MQ_TRUE) {
            g_done = MQ_FALSE;
            break ;
        }
    }

    /* Close the session */

    MQ_ERR_CHK( MQCloseSession(session) );

    if (g_err == MQ_TRUE) {
        printf("async_jmsserver: ARECVMESSAGES end unsuccessfully \n");
        g_err == MQ_FALSE;
	    tpreturn(TPFAIL, -1, NULL, 0L, 0);
    }

    printf("async_jmsserver: ARECVMESSAGES end successfully \n");

    tpreturn(TPSUCCESS, MQ_OK, NULL, 0L, 0);


Cleanup:
    {
    MQString estr = MQGetStatusString(status);
    printf("async_jmsserver: Error: %s\n", (estr == NULL) ? "NULL":estr);
    MQFreeString(estr);
    }
    MQCloseSession(session);
    MQFreeDestination(queue);
    tpreturn(TPFAIL, -1, NULL, 0L, 0);
}


static MQInt32  msg_count = 0;

MQError 
beforeMessageDelivery(const MQSessionHandle sessionHandle,
                      const MQConsumerHandle consumerHandle,
                      MQMessageHandle message,
                      MQError mqerror,
                      void * callbackData) 
{

    TPCONTEXT_T oldcxt, *newcxt = (TPCONTEXT_T *)callbackData;

    if (tpgetctxt(&oldcxt, 0) < 0) { 
        printf("ERROR: async_jmsserver:beforeMessageDelivery: \
                       tpgetctxt() failed: tperrno=%d\n", tperrno);
        return MQ_CALLBACK_RUNTIME_ERROR; 
    }

    if (tpsetctxt(*newcxt, 0) < 0) {
        printf("ERROR: async_jmsserver:beforeMessageDelivery: \
                       tpsetctxt(%ld) failed: tperrno=%d\n", *newcxt, tperrno);
        return  MQ_CALLBACK_RUNTIME_ERROR; 
    }

    /* Start a XA transaction */ 

    if (tpbegin(60, 0) == -1) {
        (void)printf("ERROR: async_jmssserver:beforeMessageDelivery: \
                             tpbegin() failed, %s\n", tpstrerror(tperrno));
        return  MQ_CALLBACK_RUNTIME_ERROR; 
    }
    printf("async_jmsserver:beforeMessageDelivery: tpbegin() success\n");

    return MQ_OK;
}


static MQBool g_end = MQ_FALSE;

MQError
afterMessageDelivery(const MQSessionHandle sessionHandle,
                     const MQConsumerHandle consumerHandle,
                     MQMessageHandle message,
                     MQError mqerror,
                     void * callbackData) 
{

    /* Check mqerror */

    if (mqerror !=  MQ_OK) {
        if (tpgetlev() != 0) {
            printf("async_jmssever:afterMessageDelivery: \
                    Message processing failed with  mqerror=%d, \
                    abort the transaction\n", mqerror);
            if (tpabort(0) == -1) {
                printf("async_jmssever:afterMessageDelivery: \
                        tpabort() failed: tperrno=%d\n", tperrno);
            }
        }
        g_done = MQ_TRUE;
        g_err = MQ_TRUE;
        return MQ_CALLBACK_RUNTIME_ERROR;
    }

    /* Commit the transaction */

    if (tpcommit(0) == -1) {
        printf("async_jmssever:afterMessageDelivery: \
                        tpcommit() failed: %s\n", tpstrerror(tperrno));
        g_done = MQ_TRUE;
        g_err = MQ_TRUE;
        return MQ_CALLBACK_RUNTIME_ERROR;
    }

    printf("async_jmsserver:afterMessageDelivery: tpcommit() success\n");

    if (tpsetctxt(TPNULLCONTEXT, 0) < 0) {
        printf("ERROR: async_jmsserver:afterMessageDelivery: \
                       tpsetctxt(%ld) failed: tperrno=%d\n", TPNULLCONTEXT, tperrno);
        g_done = MQ_TRUE;
        g_err = MQ_TRUE;
        return  MQ_CALLBACK_RUNTIME_ERROR;
    }
    if (g_end == MQ_TRUE) g_done = MQ_TRUE;

    return MQ_OK;
}


MQError 
messageListenerFunc(const MQSessionHandle sessionHandle,
                    const MQConsumerHandle consumerHandle,
                    MQMessageHandle message,
                    void * callbackData) 
{
    ConstMQString text;
    MQMessageType type;
    MQStatus status;

    printf("async_jmsserver:messageListenerFunc: msg_count=%d\n", msg_count);

    msg_count++;

    if (tpgetlev() == 0) {
        printf("ERROR: async_jmsserver:messageListenerFunc: not in transaction\n");
        MQFreeMessage(message);
        g_end = MQ_TRUE;
        g_err = MQ_TRUE;
        return MQ_CALLBACK_RUNTIME_ERROR;
    }

    MQ_ERR_CHK( MQGetMessageType(message, &type) );
    if (type == MQ_TEXT_MESSAGE) {
       MQ_ERR_CHK(MQGetTextMessageText(message, &text));
       printf("async_jmsserver:messageListenerFunc: Received message %s\n", text);
       if (strcmp(text, "END") == 0) {
           g_end = MQ_TRUE;
       }
    } else {
        printf("Received unexpected message type.\n");
        g_end = MQ_TRUE;
        g_err = MQ_TRUE;
    }

    MQ_ERR_CHK( MQFreeMessage(message) );
    return MQ_OK;

Cleanup:
    {
    MQString emsg = MQGetStatusString(status);
    printf("ERROR: async_jmsserver:messageListenerFunc: %s\n",
                  ((emsg == NULL) ? "NULL":emsg));
    MQFreeString(emsg);
    }
    MQFreeMessage(message);
    return MQGetStatusCode(status);
}

