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
 * Start of xa.h header
 *
 * Define a symbol to prevent multiple inclusions of tis header file
 */
#ifndef XA_H
#define XA_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Transaction branch identification: XID and NULLXID:
 */
#define XIDDATASIZE	128		/* size in bytes */
#define MAXGTRIDSIZE    64  		/* maximum size in bytes of gtrid */
#define MAXBQUALSIZE    64  		/* maximum size in bytes of bqual */
struct xid_t {
	long formatID;			/* format identifier */
	long gtrid_length;		/* value not to exceed 64 */
	long bqual_length;		/* value not to exceed 64 */
	char data[XIDDATASIZE];
  };
typedef struct xid_t XID;
/*
 * A value of -1 in formatID means that the XID is null.
 */
/*
 * Declarations of routines by which RMs call TMs:
 */

#ifdef __STDC__
extern int ax_reg(int, XID *, long);
extern int ax_unreg(int, long);
#else /* __STDC__ */
extern int ax_reg();
extern int ax_unreg();
#endif /* __STDC__ */

/*
 * XA Switch Data Structure
 */
#define RMNAMESZ 32		/* length of resource manager name, */
				/* including the null terminator */
#define MAXINFOSIZE 256		/* maximum size in bytes of xa_info strings, */
				/* including the null terminator */
struct xa_switch_t {
  char name[RMNAMESZ];		/* name of resource manager */
  long flags;			/* resource manager specific options */
  long version;			/* must be 0 */
#ifdef __STDC__
  int (*xa_open_entry)(char *, int, long);	/* xa_open function pointer */
  int (*xa_close_entry)(char *, int, long);	/* xa_close function pointer*/
  int (*xa_start_entry)(XID *, int, long);	/* xa_start function pointer */
  int (*xa_end_entry)(XID *, int, long);	/* xa_end function pointer */
  int (*xa_rollback_entry)(XID *, int, long);	/* xa_rollback function pointer */
  int (*xa_prepare_entry)(XID *, int, long);	/* xa_prepare function pointer */
  int (*xa_commit_entry)(XID *, int, long);	/* xa_commit function pointer */
  int (*xa_recover_entry)(XID *, long, int, long);
		/* xa_recover function pointer*/
  int (*xa_forget_entry)(XID *, int, long);	/* xa_forget function pointer */
  int (*xa_complete_entry)(int *, int *, int, long);
		/* xa_complete function pointer */
#else /* __STDC__ */
  int (*xa_open_entry)();	/* xa_open function pointer */
  int (*xa_close_entry)();	/* xa_close function pointer */
  int (*xa_start_entry)();	/* xa_start function pointer */
  int (*xa_end_entry)();	/* xa_end function pointer */
  int (*xa_rollback_entry)();	/* xa_rollback function pointer */
  int (*xa_prepare_entry)();	/* xa_prepare function pointer */
  int (*xa_commit_entry)();	/* xa_commit function pointer */
  int (*xa_recover_entry)();	/* xa_recover function pointer */
  int (*xa_forget_entry)();	/* xa_forget function pointer */
  int (*xa_complete_entry)();	/* xa_complete function pointer */
#endif /* __STDC__ */
};


/*
 * Flag definitions for the RM switch
 */
#define TMNOFLAGS	0x00000000L	/* no resource manager features
					 * selected
					 */
#define TMREGISTER	0x00000001L	/* resource manager dynamically
					 * registers
					 */
#define TMNOMIGRATE	0x00000002L	/* resource manager does not support
					 * association migration
					 */
#define TMUSEASYNC	0x00000004L	/* resource manager supports
					 * asynchronous operations
					 */
/*
 * Flag definitions for xa_ and ax_ routines
 */
/* use TMNOFLAGS, defined above, when not specifying other flags */
#define TMASYNC		0x80000000L	/* perform routine asynchronously */
#define TMONEPHASE	0x40000000L	/* caller is using one-phase commit
					 * optimisation
					 */
#define TMFAIL		0x20000000L	/* dissociates caller and marks
					 * transaction branch rollback-only
					 */
#define TMNOWAIT	0x10000000L	/* return if blocking condition exists */
#define TMRESUME	0x08000000L	/* caller is resuming association
					 * with suspended transaction branch
					 */
#define TMSUCCESS	0x04000000L	/* dissociate caller from transaction
					 * branch
					 */
#define TMSUSPEND	0x02000000L	/* caller is suspending, not ending,
					 * association
					 */
#define TMSTARTRSCAN	0x01000000L	/* start a recovery scan */
#define TMENDRSCAN	0x00800000L	/* end a recovery scan */
#define TMMULTIPLE	0x00400000L	/* wait for any asynchronous operation */
#define TMJOIN		0x00200000L	/* caller is joining existing
					 * transaction branch
					 */
#define TMMIGRATE	0x00100000L	/* caller intends to perform migration */
/*
 * ax_() return codes (transaction manager reports to resource manager)
 */
#define TM_JOIN		2	/* caller is joining existing transaction
				 * branch
				 */
#define TM_RESUME	1	/* caller is resuming association with
				 * suspended transaction branch
				 */
#define TM_OK		0	/* normal execution */
#define TMER_TMERR	-1	/* an error occurred in the
				 * transaction manager
				 */
#define TMER_INVAL	-2	/* invalid arguments were given */
#define TMER_PROTO	-3	/* routine invoked in an improper context */
/*
 * xa_() return codes (resource manager reports to transaction manager)
 */
#define XA_RBBASE	100		/* The inclusive lower bound of the
					 * rollback codes
					 */
#define XA_RBROLLBACK	XA_RBBASE	/* The rollback was caused by an
					 * unspecified reason
					 */
#define XA_RBCOMMFAIL	XA_RBBASE+1	/* The rollback was caused by a
					 * communication failure
					 */
#define XA_RBDEADLOCK	XA_RBBASE+2	/* A deadlock was detected */
#define XA_RBINTEGRITY	XA_RBBASE+3	/* A condition that violates the integrity
					 * of the resources was detected
					 */
#define XA_RBOTHER	XA_RBBASE+4	/* The resource manager rolled back the
					 * transaction branch for a reason not
					 * on this list
					 */
#define XA_RBPROTO	XA_RBBASE+5	/* A protocol error occurred in the
					 * resource manager
					 */
#define XA_RBTIMEOUT	XA_RBBASE+6	/* A transaction branch took too long */
#define XA_RBTRANSIENT	XA_RBBASE+7	/* May retry the transaction branch */
#define XA_RBEND	XA_RBTRANSIENT	/* The inclusive upper bound of the
					 * rollback codes
					 */

#define XA_NOMIGRATE	9	/* resumption must occur where
				 * suspension occurred
				 */
#define XA_HEURHAZ	8	/* the transaction branch may have
				 * been heuristically completed
				 */
#define XA_HEURCOM	7	/* the transaction branch has been
				 * heuristically committed
				 */
#define XA_HEURRB	6	/* the transaction branch has been
				 * heuristically rolled back
				 */
#define XA_HEURMIX	5	/* the transaction branch has been
				 * heuristically committed and rolled back
				 */
#define XA_RETRY	4	/* routine returned with no effect and
				 * may be re-issued
				 */
#define XA_RDONLY	3	/* the transaction branch was read-only and
				 * has been committed
				 */
#define XA_OK		0	/* normal execution */
#define XAER_ASYNC	-2	/* asynchronous operation already outstanding */
#define XAER_RMERR	-3	/* a resource manager error occurred in the
				 * transaction branch
				 */
#define XAER_NOTA	-4	/* the XID is not valid */
#define XAER_INVAL	-5	/* invalid arguments were given */
#define XAER_PROTO	-6	/* routine invoked in an improper context */
#define XAER_RMFAIL	-7	/* resource manager unavailable */
#define XAER_DUPID	-8	/* the XID already exists */
#define XAER_OUTSIDE	-9	/* resource manager doing work outside */
				/* global transaction */
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* XA_H */
/*
 * End of xa.h header
 */
