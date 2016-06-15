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
#include <atmi.h>      /* TUXEDO */
#include <userlog.h>   /* TUXEDO */
#include <mqcrt.h> 


#define MQ_ERR_CHK(mqCall)                             \
  if (MQStatusIsError(status = (mqCall)) == MQ_TRUE) { \
    goto Cleanup;                                      \
  }


/* 
 * This function performs the actual service requested by the client.
 * Its argument is a structure containing among other things a pointer
 * to the data buffer, and the length of the data buffer.
 */

/* for this example we ignore rqst */

void
SENDMESSAGES(TPSVCINFO *rqst)
{
    MQConnectionHandle connection = MQ_INVALID_HANDLE;
    MQSessionHandle session = MQ_INVALID_HANDLE;
    MQDestinationHandle queue = MQ_INVALID_HANDLE;
    MQProducerHandle producer = MQ_INVALID_HANDLE;
    MQMessageHandle message = MQ_INVALID_HANDLE;
    MQStatus status;

    int maxNumMsgs = 10;
    ConstMQString text = "This is a message";


    printf("jmsserver: SENDMESSAGES started\n");

    /* Get XA Connection */

    MQ_ERR_CHK( MQGetXAConnection(&connection) );

    /* Create a XA Session if in transaction else create a regular Session */

    if (tpgetlev() != 0) {
        printf("jmsserver: Creating XA session\n");
        MQ_ERR_CHK( MQCreateXASession(connection, MQ_SESSION_SYNC_RECEIVE,
                                      NULL, NULL, NULL, &session) );
    } else {
        printf("jmsserver: Creating non-XA session \n");
        MQ_ERR_CHK( MQCreateSession(connection, MQ_FALSE, MQ_AUTO_ACKNOWLEDGE,
                                    MQ_SESSION_SYNC_RECEIVE, &session) );
    }
    printf("jmsserver: Created Session successfully\n");

    /* Create a Destination */

    MQ_ERR_CHK( MQCreateDestination(session, "xatestqueue", MQ_QUEUE_DESTINATION, &queue) );
    printf("jmsserver: Created destination successfully\n");

    /* Create a Message Producer */

	MQ_ERR_CHK( MQCreateMessageProducerForDestination(session, queue, &producer) );
    printf("jmsserver: Created producer successfully\n");

    MQ_ERR_CHK( MQFreeDestination(queue) );


	/* Send Messages */

    for (int i = 0; i < maxNumMsgs; i++) {

        MQ_ERR_CHK( MQCreateTextMessage(&message) );
        if (i == (maxNumMsgs -1)) {
		    MQ_ERR_CHK( MQSetTextMessageText(message, "END") );
        } else {
		    MQ_ERR_CHK( MQSetTextMessageText(message, text) );
        }
        printf("jmsserver: Sending message i=%d\n", i);
		MQ_ERR_CHK( MQSendMessage(producer, message) );
        printf("jmsserver: Sent message i=%d\n", i);
		MQ_ERR_CHK( MQFreeMessage(message) );
	}

    /* Close the Session */

	MQ_ERR_CHK( MQCloseSession(session) );

	printf("jmsserver: SENDMESSAGES end\n");

	tpreturn(TPSUCCESS, MQ_OK, NULL, 0L, 0);

Cleanup:    
    {
    MQString estr = MQGetStatusString(status);
    printf("jmsserver: Error: %s\n", (estr == NULL) ? "NULL":estr);
    MQFreeString(estr);
    }
    MQCloseSession(session);
    MQFreeDestination(queue);
    MQFreeMessage(message);

    tpreturn(TPFAIL, -1, NULL, 0L, 0);
}


void
RECVMESSAGES(TPSVCINFO *rqst)
{
    MQConnectionHandle connection = MQ_INVALID_HANDLE;
    MQSessionHandle session = MQ_INVALID_HANDLE;
    MQDestinationHandle queue = MQ_INVALID_HANDLE;
    MQConsumerHandle consumer = MQ_INVALID_HANDLE;
    MQMessageHandle message = MQ_INVALID_HANDLE;
    MQStatus status;

    ConstMQString text;
	int maxNumMsgs = 10;

    printf("jmsserver: RECVMESSAGES started\n");

    /* Get XA Connection */

    MQ_ERR_CHK( MQGetXAConnection(&connection) );

    /* Create a XA Session if in transaction else create a regular Session */

    if (tpgetlev() != 0) {
        printf("jmsserver: Creating XA session\n");
        MQ_ERR_CHK( MQCreateXASession(connection, MQ_SESSION_SYNC_RECEIVE,
                                                NULL, NULL, NULL, &session) );
    } else {
	    printf("jmsserver: Creating non-XA session\n");
        MQ_ERR_CHK( MQCreateSession(connection, MQ_FALSE, MQ_AUTO_ACKNOWLEDGE,
                                             MQ_SESSION_SYNC_RECEIVE, &session) );
    }
    printf("jmsserver: Created session successfully\n");

    /* Create a Destination */
    MQ_ERR_CHK( MQCreateDestination(session,"xatestqueue", MQ_QUEUE_DESTINATION, &queue) );
    printf("jmsserver: Created queue successfully\n");

    /* Create a Message Consumer */

    MQ_ERR_CHK( MQCreateMessageConsumer(session, queue, NULL, MQ_FALSE, &consumer) );
    printf("jmsserver: Created consumer successfully\n");

    MQ_ERR_CHK( MQFreeDestination(queue) );

    /* Start the Connection */

	MQ_ERR_CHK( MQStartConnection(connection) );
	printf("jmsserver: Started connection successfully\n");

    /* Receive Messages */

    for (int i = 0; i < maxNumMsgs; i++) {
        printf("jmsserver: Waiting (30sec) for messages ...\n");
        MQ_ERR_CHK( MQReceiveMessageWithTimeout(consumer, 30000, &message) );
        printf("jmsserver: Received %dth message: ", i);
        MQ_ERR_CHK( MQGetTextMessageText(message, &text) );
        printf("%s\n", text);
		MQ_ERR_CHK( MQFreeMessage(message) );
    }

    /* Close the Session */

	MQ_ERR_CHK( MQCloseSession(session) );
    printf("Closed session successfully\n");

    printf("jmsserver: RECVMESSAGES end\n");

    tpreturn(TPSUCCESS, MQ_OK, NULL, 0L, 0);

Cleanup:
    {
    MQString estr = MQGetStatusString(status);
    printf("jmsserver: Error: %s\n", (estr == NULL) ? "NULL":estr);
    MQFreeString(estr);
    }
    MQCloseSession(session);
    MQFreeMessage(message);
    MQFreeDestination(queue);

    tpreturn(TPFAIL, -1, NULL, 0L, 0);
}

