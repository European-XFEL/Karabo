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
 * @(#)PacketType.hpp	1.9 10/17/07
 */ 

#ifndef PACKETTYPE_HPP
#define PACKETTYPE_HPP

#include <nspr.h>


 /* Body types used by some control packets */

static const PRUint32 BODY_TYPE_NONE                        = 0;
static const PRUint32 BODY_TYPE_CONSUMERID_I_SYSMESSAGEID   = 1;
static const PRUint32 BODY_TYPE_CONSUMERID_L_SYSMESSAGEID   = 2;
static const PRUint32 BODY_TYPE_SYSMESSAGEID                = 3;
static const PRUint32 BODY_TYPE_SESSIONID_SYSMESSAGEID      = 4;


/*
* Our loose convention is that the first 8 types are reserved for
* basic JMS message types (of which there are currently 6). After
* that even types are requests and odd types are replies. That's why
* you'll see some holes in the sequence for requests that don't
* have replies.
*/
static const PRUint16 PACKET_TYPE_INVALID                   =  0;

static const PRUint16 PACKET_TYPE_TEXT_MESSAGE              =  1;
static const PRUint16 PACKET_TYPE_BYTES_MESSAGE             =  2;
static const PRUint16 PACKET_TYPE_MAP_MESSAGE               =  3;
static const PRUint16 PACKET_TYPE_STREAM_MESSAGE            =  4;
static const PRUint16 PACKET_TYPE_OBJECT_MESSAGE            =  5;
static const PRUint16 PACKET_TYPE_MESSAGE                   =  6;

static const PRUint16 PACKET_TYPE_SEND_REPLY                =  9;

static const PRUint16 PACKET_TYPE_HELLO                     = 10;
static const PRUint16 PACKET_TYPE_HELLO_REPLY               = 11;
static const PRUint16 PACKET_TYPE_AUTHENTICATE              = 12;
static const PRUint16 PACKET_TYPE_AUTHENTICATE_REPLY        = 13;
static const PRUint16 PACKET_TYPE_ADD_CONSUMER              = 14;
static const PRUint16 PACKET_TYPE_ADD_CONSUMER_REPLY        = 15;
static const PRUint16 PACKET_TYPE_DELETE_CONSUMER           = 16;
static const PRUint16 PACKET_TYPE_DELETE_CONSUMER_REPLY     = 17;
static const PRUint16 PACKET_TYPE_ADD_PRODUCER              = 18;
static const PRUint16 PACKET_TYPE_ADD_PRODUCER_REPLY        = 19;
static const PRUint16 PACKET_TYPE_START                     = 20;

static const PRUint16 PACKET_TYPE_STOP                      = 22;
static const PRUint16 PACKET_TYPE_STOP_REPLY                = 23;
static const PRUint16 PACKET_TYPE_ACKNOWLEDGE               = 24;
static const PRUint16 PACKET_TYPE_ACKNOWLEDGE_REPLY         = 25;
static const PRUint16 PACKET_TYPE_BROWSE                    = 26;
static const PRUint16 PACKET_TYPE_BROWSE_REPLY              = 27;
static const PRUint16 PACKET_TYPE_MESSAGE_SET               = 27; // for bkwds compat
static const PRUint16 PACKET_TYPE_GOODBYE                   = 28;
static const PRUint16 PACKET_TYPE_GOODBYE_REPLY             = 29;

static const PRUint16 PACKET_TYPE_ERROR                     = 30;

static const PRUint16 PACKET_TYPE_REDELIVER                 = 32;

static const PRUint16 PACKET_TYPE_CREATE_DESTINATION        = 34;
static const PRUint16 PACKET_TYPE_CREATE_DESTINATION_REPLY  = 35;
static const PRUint16 PACKET_TYPE_DESTROY_DESTINATION       = 36;
static const PRUint16 PACKET_TYPE_DESTROY_DESTINATION_REPLY = 37;
static const PRUint16 PACKET_TYPE_AUTHENTICATE_REQUEST      = 38;

static const PRUint16 PACKET_TYPE_VERIFY_DESTINATION        = 40;
static const PRUint16 PACKET_TYPE_VERIFY_DESTINATION_REPLY  = 41;
static const PRUint16 PACKET_TYPE_DELIVER                   = 42;
static const PRUint16 PACKET_TYPE_DELIVER_REPLY             = 43;
static const PRUint16 PACKET_TYPE_START_TRANSACTION         = 44;
static const PRUint16 PACKET_TYPE_START_TRANSACTION_REPLY   = 45;
static const PRUint16 PACKET_TYPE_COMMIT_TRANSACTION        = 46;
static const PRUint16 PACKET_TYPE_COMMIT_TRANSACTION_REPLY  = 47;
static const PRUint16 PACKET_TYPE_ROLLBACK_TRANSACTION      = 48;
static const PRUint16 PACKET_TYPE_ROLLBACK_TRANSACTION_REPLY= 49;

static const PRUint16 PACKET_TYPE_SET_CLIENTID              = 50;
static const PRUint16 PACKET_TYPE_SET_CLIENTID_REPLY        = 51;

static const PRUint16 PACKET_TYPE_RESUME_FLOW               = 52;

static const PRUint16 PACKET_TYPE_PING                      = 54;
static const PRUint16 PACKET_TYPE_PING_REPLY                = 55;

static const PRUint16 PACKET_TYPE_PREPARE_TRANSACTION       = 56;
static const PRUint16 PACKET_TYPE_PREPARE_TRANSACTION_REPLY = 57;

static const PRUint16 PACKET_TYPE_END_TRANSACTION           = 58;
static const PRUint16 PACKET_TYPE_END_TRANSACTION_REPLY     = 59;

static const PRUint16 PACKET_TYPE_RECOVER_TRANSACTION       = 60;
static const PRUint16 PACKET_TYPE_RECOVER_TRANSACTION_REPLY = 61;

static const PRUint16 PACKET_TYPE_GENERATE_UID              = 62;
static const PRUint16 PACKET_TYPE_GENERATE_UID_REPLY        = 63;

static const PRUint16 PACKET_TYPE_FLOW_PAUSED               = 64;

static const PRUint16 PACKET_TYPE_DELETE_PRODUCER           = 66;
static const PRUint16 PACKET_TYPE_DELETE_PRODUCER_REPLY     = 67;

static const PRUint16 PACKET_TYPE_CREATE_SESSION            = 68;
static const PRUint16 PACKET_TYPE_CREATE_SESSION_REPLY      = 69;

static const PRUint16 PACKET_TYPE_DESTROY_SESSION           = 70;
static const PRUint16 PACKET_TYPE_DESTROY_SESSION_REPLY     = 71;

static const PRUint16 PACKET_TYPE_GET_INFO                  = 72;
static const PRUint16 PACKET_TYPE_GET_INFO_REPLY            = 73;

static const PRUint16 PACKET_TYPE_DEBUG                     = 74;


static const PRUint16 PACKET_TYPE_LAST                      = 75;


/**
 */
static const PRUint32 PROTOCOL_VERSION                      = 500; 

/**
 * This class defines the constants for MQ packet types.
 * 
 * The constants were copied out of the corresponding Java class file, PacketType.java
 */
class PacketType {
public:
  static const char * toString(const PRUint16 packetType);
};


#endif // PACKETTYPE_HPP
