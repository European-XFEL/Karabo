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
 * @(#)mqconnection-props.h	1.17 06/26/07
 */ 

#ifndef MQ_CONNECTION_PROPERTIES_H
#define MQ_CONNECTION_PROPERTIES_H

/*
 * defines constants for connection properties
 */

static const char * MQ_BROKER_HOST_PROPERTY             = "MQBrokerHostName"; /* MQString */
static const char * MQ_BROKER_PORT_PROPERTY             = "MQBrokerHostPort"; /* MQInt32  */
static const char * MQ_ENABLE_IPV6_PROPERTY                = "MQEnableIPv6"; /* MQBool  */
static const char * MQ_SERVICE_PORT_PROPERTY            = "MQServicePort";    /* MQInt32  */
static const char * MQ_CONNECTION_TYPE_PROPERTY         = "MQConnectionType"; /* MQString */
static const char * MQ_ACK_TIMEOUT_PROPERTY             = "MQAckTimeout";     /* MQInt32 in millisecond */
static const char * MQ_ACK_ON_PRODUCE_PROPERTY          = "MQAckOnProduce";        /* MQBool */
static const char * MQ_ACK_ON_ACKNOWLEDGE_PROPERTY      = "MQAckOnAcknowledge";    /* MQBool */
static const char * MQ_CONNECTION_FLOW_COUNT_PROPERTY         = "MQConnectionFlowCount";        /* MQInt32 */
static const char * MQ_CONNECTION_FLOW_LIMIT_ENABLED_PROPERTY = "MQConnectionFlowLimitEnabled"; /* MQBool  */
static const char * MQ_CONNECTION_FLOW_LIMIT_PROPERTY         = "MQConnectionFlowLimit";        /* MQInt32 */
static const char * MQ_PING_INTERVAL_PROPERTY           = "MQPingInterval";   /* MQInt32 in second */


/** SSL */
static const char * MQ_SSL_BROKER_IS_TRUSTED            = "MQSSLIsHostTrusted";        /* MQBool */
static const char * MQ_SSL_CHECK_BROKER_FINGERPRINT     = "MQSSLCheckHostFingerprint"; /* MQBool */
static const char * MQ_SSL_BROKER_CERT_FINGERPRINT      = "MQSSLHostCertFingerprint";  /* MQString */


/** connection metadata properties to be used with MQGetMetaData   */
static const char * MQ_NAME_PROPERTY            = "MQ_NAME";
static const char * MQ_VERSION_PROPERTY         = "MQ_VERSION";
static const char * MQ_MAJOR_VERSION_PROPERTY   = "MQ_VMAJOR";
static const char * MQ_MINOR_VERSION_PROPERTY   = "MQ_VMINOR";
static const char * MQ_MICRO_VERSION_PROPERTY   = "MQ_VMICRO";
static const char * MQ_SERVICE_PACK_PROPERTY    = "MQ_SVCPACK";
static const char * MQ_UPDATE_RELEASE_PROPERTY  = "MQ_URELEASE";

#endif /* MQ_CONNECTION_PROPERTIES_H */
