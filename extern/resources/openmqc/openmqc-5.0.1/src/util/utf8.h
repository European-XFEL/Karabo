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
 * @(#)utf8.h	1.3 06/26/07
 */ 

#ifndef TEMP_UTF8_H
#define TEMP_UTF8_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*
 * UTF-8 routines (should these move into libnls?)
 */
/* number of bytes in character */
int  ldap_utf8len( const char* );

/* find next character */
char*  ldap_utf8next( char* );

/* find previous character */
char*  ldap_utf8prev( char* );

/* copy one character */
int  ldap_utf8copy( char* dst, const char* src );

/* total number of characters */
size_t  ldap_utf8characters( const char* );

/* get one UCS-4 character, and move *src to the next character */
unsigned long  ldap_utf8getcc( const char** src );

/* UTF-8 aware strtok_r() */
  // char*  ldap_utf8strtok_r( char* src, const char* brk, char** next);

/* like isalnum(*s) in the C locale */
  // int  ldap_utf8isalnum( char* s );
/* like isalpha(*s) in the C locale */
  // int  ldap_utf8isalpha( char* s );
/* like isdigit(*s) in the C locale */
  // int  ldap_utf8isdigit( char* s );
/* like isxdigit(*s) in the C locale */
  // int  ldap_utf8isxdigit(char* s );
/* like isspace(*s) in the C locale */
  // int  ldap_utf8isspace( char* s );


/* UTF8 related prototypes: put in the header file of your choice (ldap.h)*/
  // int ldap_has8thBit(const unsigned char *s);
  // unsigned char *ldap_utf8StrToLower(const unsigned char *s);
  // void ldap_utf8ToLower(unsigned char *s, unsigned char *d, int *ssz, int *dsz);
  // int ldap_utf8isUpper(unsigned char *s);
  // unsigned char *ldap_utf8StrToUpper(unsigned char *s);
  // void ldap_utf8ToUpper(unsigned char *s, unsigned char *d, int *ssz, int *dsz);
  // int ldap_utf8isLower(unsigned char *s);
  //int ldap_utf8casecmp(const unsigned char *s0, const unsigned char *s1);
  //int ldap_utf8ncasecmp(const unsigned char *s0, const unsigned char *s1, int n);


#ifdef __cplusplus
}
#endif

#endif
