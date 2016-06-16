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
 * @(#)PacketType.cpp	1.6 06/26/07
 */ 

#include "PacketType.hpp"

static const char * PACKET_TYPE_STRINGS[] = {
  "INVALID",                      //  INVALID                   =  0;
  "TEXT_MESSAGE",                 //  TEXT_MESSAGE              =  1;
  "BYTES_MESSAGE",                //  BYTES_MESSAGE             =  2;
  "MAP_MESSAGE",                  //  MAP_MESSAGE               =  3;
  "STREAM_MESSAGE",               //  STREAM_MESSAGE            =  4;
  "OBJECT_MESSAGE",               //  OBJECT_MESSAGE            =  5;
  "MESSAGE",                      //  MESSAGE                   =  6;
  "INVALID",                      //  INVALID                   =  7;
  "INVALID",                      //  INVALID                   =  8;
  "SEND_REPLY",                   //  SEND_REPLY                =  9;
  "HELLO",                        //  HELLO                     = 10;
  "HELLO_REPLY",                  //  HELLO_REPLY               = 11;
  "AUTHENTICATE",                 //  AUTHENTICATE              = 12;
  "AUTHENTICATE_REPLY",           //  AUTHENTICATE_REPLY        = 13;
  "ADD_CONSUMER",                 //  ADD_CONSUMER              = 14;
  "ADD_CONSUMER_REPLY",           //  ADD_CONSUMER_REPLY        = 15;
  "DELETE_CONSUMER",              //  DELETE_CONSUMER           = 16;
  "DELETE_CONSUMER_REPLY",        //  DELETE_CONSUMER_REPLY     = 17;
  "ADD_PRODUCER",                 //  ADD_PRODUCER              = 18;
  "ADD_PRODUCER_REPLY",           //  ADD_PRODUCER_REPLY        = 19;
  "START",                        //  START                     = 20;
  "INVALID",                      //  INVALID                   = 21;
  "STOP",                         //  STOP                      = 22;
  "STOP_REPLY",                   //  STOP_REPLY                = 23;
  "ACKNOWLEDGE",                  //  ACKNOWLEDGE               = 24;
  "ACKNOWLEDGE_REPLY",            //  ACKNOWLEDGE_REPLY         = 25;
  "BROWSE",                       //  BROWSE                    = 26;
  "BROWSE_REPLY",                 //  BROWSE_REPLY              = 27;
  "GOODBYE",                      //  GOODBYE                   = 28;
  "GOODBYE_REPLY",                //  GOODBYE_REPLY             = 29;
  "ERROR",                        //  ERROR                     = 30;
  "INVALID",                      //  INVALID                   = 31;
  "REDELIVER",                    //  REDELIVER                 = 32;
  "INVALID",                      //  INVALID                   = 33;
  "CREATE_DESTINATION",           //  CREATE_DESTINATION        = 34;
  "CREATE_DESTINATION_REPLY",     //  CREATE_DESTINATION_REPLY  = 35;
  "DESTROY_DESTINATION",          //  DESTROY_DESTINATION       = 36;
  "DESTROY_DESTINATION_REPLY",    //  DESTROY_DESTINATION_REPLY = 37;
  "AUTHENTICATE_REQUEST",         //  AUTHENTICATE_REQUEST      = 38;
  "INVALID",                      //  INVALID                   = 39;
  "VERIFY_DESTINATION",           //  VERIFY_DESTINATION        = 40;
  "VERIFY_DESTINATION_REPLY",     //  VERIFY_DESTINATION_REPLY  = 41;
  "DELIVER",                      //  DELIVER                   = 42;
  "DELIVER_REPLY",                //  DELIVER_REPLY             = 43;
  "START_TRANSACTION",            //  START_TRANSACTION         = 44;
  "START_TRANSACTION_REPLY",      //  START_TRANSACTION_REPLY   = 45;
  "COMMIT_TRANSACTION",           //  COMMIT_TRANSACTION        = 46;
  "COMMIT_TRANSACTION_REPLY",     //  COMMIT_TRANSACTION_REPLY  = 47;
  "ROLLBACK_TRANSACTION",         //  ROLLBACK_TRANSACTION      = 48;
  "ROLLBACK_TRANSACTION_REPLY",   //  ROLLBACK_TRANSACTION_REPLY= 49;
  "SET_CLIENTID",                 //  SET_CLIENTID              = 50;
  "SET_CLIENTID_REPLY",           //  SET_CLIENTID_REPLY        = 51;
  "RESUME_FLOW",                  //  RESUME_FLOW               = 52;
  "INVALID",                      //  INVALID                   = 53;
  "PING",                         //  PING                      = 54;
  "PING_REPLY",                   //  PING_REPLY                = 55;
  "PREPARE_TRANSACTION",          //  PREPARE_TRANSACTION       = 56;
  "PREPARE_TRANSACTION_REPLY",    //  PREPARE_TRANSACTION_REPLY = 57;
  "END_TRANSACTION",              //  END_TRANSACTION           = 58;
  "END_TRANSACTION_REPLY",        //  END_TRANSACTION_REPLY     = 59;
  "RECOVER_TRANSACTION",          //  RECOVER_TRANSACTION       = 60;
  "RECOVER_TRANSACTION_REPLY",    //  RECOVER_TRANSACTION_REPLY = 61;
  "GENERATE_UID",                 //  GENERATE_UID              = 62;
  "GENERATE_UID_REPLY",           //  GENERATE_UID_REPLY        = 63;
  "FLOW_PAUSED",                  //  FLOW_PAUSED               = 64;
  "INVALID",                      //  INVALID                   = 65;
  "DELETE_PRODUCER",              //  DELETE_PRODUCER           = 66;
  "DELETE_PRODUCER_REPLY",        //  DELETE_PRODUCER_REPLY     = 67;
  "CREATE_SESSION",               //  CREATE_SESSION            = 68;
  "CREATE_SESSION_REPLY",         //  CREATE_SESSION_REPLY      = 69;
  "DESTROY_SESSION",              //  DESTROY_SESSION           = 70;
  "DESTROY_SESSION_REPLY",        //  DESTROY_SESSION_REPLY     = 71;
  "GET_INFO",                     //  GET_INFO                  = 72;
  "GET_INFO_REPLY",               //  GET_INFO_REPLY            = 73;
  "DEBUG",                        //  DEBUG                     = 74;

  "LAST"                          //  LAST                      = 75;
};


const int MAX_PACKET_TYPE_STR_SIZE = 50;
const char * 
PacketType::toString(const PRUint16 packetType) 
{
  static char packetTypeStr[MAX_PACKET_TYPE_STR_SIZE];
  if (packetType >= PACKET_TYPE_LAST) {
    sprintf(packetTypeStr, "INVALID (%d)", packetType);
    return packetTypeStr;
  } else {
    return PACKET_TYPE_STRINGS[packetType];
  }
}
