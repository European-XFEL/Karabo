/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 04, 2012, 11:13 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_XML_XMLDLL_HH
#define	EXFEL_XML_XMLDLL_HH

/********************************************
 *         Disable Specific Warnins         *
 ********************************************/
    

   
/********************************************
 *            Windows DLL stuff             *  
 ********************************************/
    
#if defined(_WIN32) && defined(XML_DLL)
#define DECLSPEC_XML __declspec(dllexport)
#define TEMPLATE_XML
#elif defined(unix)      || defined(__unix)      || defined(__unix__) \
 || defined(linux)       || defined(__linux)     || defined(__linux__) \
 || defined(sun)         || defined(__sun) \
 || defined(BSD)         || defined(__OpenBSD__) || defined(__NetBSD__) \
 || defined(__FreeBSD__) || defined __DragonFly__ \
 || defined(sgi)         || defined(__sgi) \
 || defined(__MACOSX__)  || defined(__APPLE__) \
 || defined(__CYGWIN__)
#define DECLSPEC_XML
#define TEMPLATE_XML
#else
#define DECLSPEC_XML __declspec(dllimport)
#define TEMPLATE_XML extern
#endif

#endif

