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
 * @(#)PRTypesUtils.h	1.3 06/26/07
 */ 

/* This file defines utilities that are used with the NSPR types.
 * 
 */

#ifndef PRTYPESUTILS_H
#define PRTYPESUTILS_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include <nspr.h>
#include <memory.h>

/* PORTABLE:  I don't know if float will always be 32 bits? */
typedef float    PRFloat32;

typedef PRUint8  UChar;

/** A 32 bit integer representation of the largest unsigned 16 bit integer.  */
static const PRInt32 MAX_PR_UINT16 = 0x0000FFFF;

/** A 32 bit integer representation of the largest signed 32 bit integer.  */
static const PRInt32 MAX_PR_INT32 = 0x7FFFFFFF;

/** A 64 bit integer representation of the largest unsigned 32 bit integer. */
static const PRUint64 LL_MAX_UINT32 = LL_INIT( 0, 0xFFFFFFFF );

/**
 * This function returns a 64 bit unsigned integer composed of hi for the 
 * high 32 bits and lo for the low 32 bits.
 * 
 * @param hi is the high 32 bits of the 64 bit integer to construct
 * @param lo is the low 32 bits of the 64 bit integer to construct
 * @return the 64 bit integer composed of hi and lo
 */
PRUint64 LL_ULLFromHiLo(const PRUint32 hi, const PRUint32 lo);


/**
 * This function breaks value64 into its 32 bit high and low parts and returns
 * the results in the output parameters hi and lo.  This function has no effect
 * if hi or lo is NULL.
 *
 * @param hi is the output parameter for the high 32 bits of value64
 * @param lo is the output parameter for the low 32 bits of value64
 * @value64 is the 64 bit integer to deconstruct into 32 bit parts
 */
void LL_HiLoFromULL(PRUint32 * const hi, 
                    PRUint32 * const lo, 
                    const PRUint64 value64);
/** 
 * This method converts the parameter, timeoutMicroSeconds, (which is a timeout
 * specified in microseconds) into a system dependent timeout specified in
 * PRIntervalTime.  The special values PR_INTERVAL_NO_WAIT and
 * PR_INTERVAL_NO_TIMEOUT are preserved.  
 *
 * @param timeoutMicroSeconds is the timeout to convert in microseconds
 * @return the timeout in PRIntervalTime units
 */
PRIntervalTime microSecondToIntervalTimeout(const PRUint32 timeoutMicroSeconds);


/**
 * This function returns the amount of timeout remaining based on when
 * the timeout was started (i.e. start), the duration of the timeout
 * (i.e. timeout), and the current time.  
 */
PRIntervalTime timeoutRemaining(const PRIntervalTime start, const PRIntervalTime timeout);

#ifdef __cplusplus
}
#endif

#endif /* PRTYPESUTILS_H */

