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
 * @(#)PRTypesUtils.c	1.3 06/26/07
 */ 

#include "PRTypesUtils.h"
#include <assert.h>

//#include "UtilityMacros.h"

/*
 * Create a PRUint64 from two PRUint32's, which represent the hi and
 * the lo part of the PRUint64.
 */
PRUint64 
LL_ULLFromHiLo(const PRUint32 hi, const PRUint32 lo)
{
  PRUint64 hiPart;
  PRUint64 loPart;
  PRUint64 result;

  LL_UI2L( hiPart, hi );
  LL_UI2L( loPart, lo );

  /* shift the hi part left by 32, and then add in the low part */
  LL_SHL( hiPart, hiPart, 32 );
  LL_ADD( result, hiPart, loPart );
  
  return result;
}

/*
 *
 */
void 
LL_HiLoFromULL(PRUint32 * const hi, 
               PRUint32 * const lo, 
               const PRUint64 value64)
{
  PRUint64 hiPart;
  PRUint64 loPart;

  if (( hi == NULL ) || ( lo == NULL )) {
    return;
  }

  /* The hiPart is value64 shifted down by 32 bits */
  LL_USHR( hiPart, value64, 32 );

  /* The loPart is value64 bitwise ANDed with 0x00000000 FFFFFFFF */
  LL_AND( loPart, value64, LL_MAX_UINT32 );

  /* Assign the 32 bit parts to hi and lo */
  LL_L2UI( *hi, hiPart );
  LL_L2UI( *lo, loPart );
}


/*
 *
 */
PRIntervalTime
microSecondToIntervalTimeout(const PRUint32 timeoutMicroSeconds)
{
  PRIntervalTime timeout = 0;
  if ((timeoutMicroSeconds == PR_INTERVAL_NO_WAIT) ||
      (timeoutMicroSeconds == PR_INTERVAL_NO_TIMEOUT))
  {
    timeout = timeoutMicroSeconds;
  } else {
    timeout = PR_MicrosecondsToInterval(timeoutMicroSeconds);
  }
 
  return timeout;
} 


PRIntervalTime
timeoutRemaining(const PRIntervalTime start, const PRIntervalTime timeout)
{
  PRIntervalTime now = 0;
  PRIntervalTime remaining  = 0;
  PRIntervalTime elapsed  = 0;

  // Special cases for no waiting, and waiting forever
  if ((timeout == PR_INTERVAL_NO_WAIT) ||
      (timeout == PR_INTERVAL_NO_TIMEOUT))
  {
    return timeout;
  }

  now = PR_IntervalNow();
  elapsed = (PRIntervalTime)(now - start);
  if (elapsed > timeout) return (PRIntervalTime)0;

  remaining = (PRIntervalTime)(timeout - elapsed);

  assert( ((PRIntervalTime)remaining < timeout) || 
          ((now == start) && ((PRIntervalTime)remaining == timeout)) );

  return (PRIntervalTime)remaining;
}
