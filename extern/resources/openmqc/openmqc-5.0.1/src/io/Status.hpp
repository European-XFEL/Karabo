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
 * @(#)Status.hpp	1.4 06/26/07
 */ 

#ifndef STATUS_HPP
#define STATUS_HPP

#include <nspr.h>
#include "../error/ErrorCodes.h"

static const PRInt32 STATUS_UNKNOWN      = 0;        // Initial value

/**
* 100-199 Informational.
* We don't have any of these yet.
*/

/**
* 200-299 Success
*/
static const PRInt32 STATUS_OK           = 200;      // Success

/**
* 300-399 Redirection
* We don't have any of these yet.
*/

/**
* 400-499 Request error
*/
static const PRInt32 STATUS_BAD_REQUEST  = 400;      // Request was invalid
static const PRInt32 STATUS_UNAUTHORIZED = 401;      // Resource requires authentication
static const PRInt32 STATUS_FORBIDDEN    = 403;	// User does not have access
static const PRInt32 STATUS_NOT_FOUND    = 404;	// Resource was not found
static const PRInt32 STATUS_NOT_ALLOWED  = 405;	// Method not allowed on resrc
static const PRInt32 STATUS_TIMEOUT      = 408;	// Server has timed out
static const PRInt32 STATUS_CONFLICT     = 409;	// Resource in conflict
static const PRInt32 STATUS_GONE         = 410;	// Resource is not available
static const PRInt32 STATUS_PRECONDITION_FAILED = 412;    // A precondition not met
static const PRInt32 STATUS_INVALID_LOGIN       = 413;    // invalid login
static const PRInt32 STATUS_RESOURCE_FULL       = 414;    // Resource is full 
static const PRInt32 STATUS_ENTITY_TOO_LARGE    = 423;    // Request entity too large

/**
* 500-599 Server error
*/
static const PRInt32 STATUS_ERROR            = 500; // Internal server error
static const PRInt32 STATUS_NOT_IMPLEMENTED  = 501; // Not implemented
static const PRInt32 STATUS_UNAVAILABLE      = 503; // Server is temporarily
// unavailable
static const PRInt32 STATUS_BAD_VERSION      = 505; // Version not supported


/**
 * This class enumerates the JMQ status codes. It roughly follows the
 * HTTP model of dividing the status codes into categories.
 *
 */
class Status {
public:

  /**
   * Converts the status to the corresponding iMQError
   */
  static iMQError toIMQError(const PRInt32 statusCode);
  
};

#endif






