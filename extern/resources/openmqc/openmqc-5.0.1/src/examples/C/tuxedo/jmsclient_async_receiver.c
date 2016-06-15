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
#include <stdlib.h>
#include "atmi.h"     /* TUXEDO */
#include "userlog.h"  /* TUXEDO */


int main(int argc, char *argv[])
{
	char *recvbuf;
	long rcvlen;
	int ret;

	/* Attach to System/T as a Client Process */
	if (tpinit((TPINIT *) NULL) == -1) 
	{
        (void) fprintf(stderr, "tpinit failed\n");
        exit(1);
	}
	
	/** documentation implies that it is not valid to have
	    odata as NULL, so allocate some arbitrary memory to 
	    keep it happy */
	if((recvbuf = (char *) tpalloc("STRING", NULL, 10)) == NULL) {
		(void) fprintf(stderr,"Error allocating receive buffer\n");
		tpterm();
		exit(1);
	}

	/* Request the service TOUPPER, waiting for a reply */
	printf("calling tpcall(ARECVMESSAGES)\n");
	ret = tpcall("ARECVMESSAGES", (char *)NULL, 0, (char **)&recvbuf, &rcvlen, (long)0);
	printf("tpcall(ARECVMESSAGES) returned %d\n",ret);

    if (ret < 0) {
		(void)fprintf(stderr, "Can't send request to service ARECVMESSAGES. tpcall return value = %d\n", ret);
		(void)fprintf(stderr, "tperrno = %d\n", tperrno);
		(void)fprintf(stderr, "Failed to call ARECVMESSAGES, \"%s\n\"",
			tpstrerror(tperrno));
		tpfree(recvbuf);
		(void)tpterm();
		exit(1);
	}

    printf("ARECVMESSAGES completed !\n");

	/* Free Buffers & Detach from System/T */
	tpfree(recvbuf);
	tpterm();
	return(0);
}
