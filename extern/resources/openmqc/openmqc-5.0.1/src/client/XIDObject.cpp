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
 * @(#)XIDObject.cpp	1.2 10/23/07
 */ 

#include "XIDObject.hpp"
#include "../util/UtilityMacros.h"

#define NULLXID     -1L
#define NULLXID_STR "NULLXID_STR" 

/**
 * Construct an unititialized XID to be used by readID
 */
XIDObject::XIDObject()
{
  CHECK_OBJECT_VALIDITY();
  this->init();
}

XIDObject::~XIDObject()
{
  CHECK_OBJECT_VALIDITY();

  reset();
}


void
XIDObject::init()
{
  CHECK_OBJECT_VALIDITY();

  xid.formatID  = NULLXID;
  xid.gtrid_length = 0L;
  xid.bqual_length =0L;
  memset(xid.data, 0, XIDDATASIZE);

  xidStr = NULL;
}

void
XIDObject::reset()
{
  CHECK_OBJECT_VALIDITY();

  DELETE( xidStr );
  init();
}


MQError
XIDObject::write(IMQDataOutputStream * const out) const
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( out );

  RETURN_IF_ERROR( out->writeUint32(xid.formatID) );
  RETURN_IF_ERROR( out->writeUint16((PRUint16)xid.gtrid_length) );
  RETURN_IF_ERROR( out->writeUint16((PRUint16)xid.bqual_length) );
  RETURN_IF_ERROR( out->writeUint8Array((PRUint8 *)xid.data, XIDDATASIZE) );
  
  return MQ_SUCCESS;
}


MQError
XIDObject::read(IMQDataInputStream * const in)
{
  CHECK_OBJECT_VALIDITY();

  MQError errorCode = MQ_SUCCESS;
  RETURN_ERROR_IF_NULL( in );
  reset();

  PRUint16 v = 0;
  ERRCHK( in->readUint32((PRUint32 *)&(xid.formatID)) );
  ERRCHK( in->readUint16(&v) );
  xid.gtrid_length = v;
  ERRCHK( in->readUint16(&v) );
  xid.bqual_length = v;
  ERRCHK( in->readUint8Array((PRUint8 *)xid.data, XIDDATASIZE) );

  return MQ_SUCCESS;

Cleanup:
  reset();
  return errorCode;
}

MQError
XIDObject::copy(const XID * xidin)
{
  CHECK_OBJECT_VALIDITY();

  RETURN_ERROR_IF_NULL( xidin );
  reset();

  xid.formatID = xidin->formatID;
  xid.gtrid_length = xidin->gtrid_length;
  xid.bqual_length = xidin->bqual_length;
  memcpy(xid.data, xidin->data, xidin->gtrid_length+xidin->bqual_length);
  return MQ_SUCCESS;
}

/**
 *
 */
XID *
XIDObject::getXID()
{
  CHECK_OBJECT_VALIDITY();

  return &(this->xid);
}


PRInt32
XIDObject::xidSize()
{
  return sizeof(PRUint32) + 2*sizeof(PRUint16)+XIDDATASIZE*sizeof(PRUint8);
}

char *
XIDObject::toString()
{
  CHECK_OBJECT_VALIDITY();

  if (xidStr != NULL) {
    return xidStr;
  }
  xidStr = XIDObject::toString(this->getXID());
  return xidStr;

}


static const char _hextab[] = "0123456789ABCDEF";

/**
 * make a null-terminated string
 * caller is responsible to free the returned string
 *
 * @return NULL if out of memory or xid is NULL
 */
char *
XIDObject::toString(XID *xidin) 
{
  char * key = NULL;

  if (xidin == NULL) return NULL;

  if (xidin->formatID == NULLXID) {
    key = new char[STRLEN(NULLXID_STR)+1];
    if (key == NULL) return NULL;

    STRCPY(key, NULLXID_STR);
    return key;
  }

  int i, j, value;
  size_t len = (size_t)(xidin->gtrid_length + xidin->bqual_length);

  key = new char[2*len+1];
  if (key == NULL) return NULL;
  memset(key, 0, 2*len+1);

  j = 0;
  for (i = 0; i < (int)len; i++) {
    value = xidin->data[i]&0xff;
    key[j++] = _hextab[value/16];
    key[j++] = _hextab[value&15];
  }
  key[2*len] = '\0';
  return key;
}

