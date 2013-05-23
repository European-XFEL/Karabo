/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 04, 2012, 11:13 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_CORE_COREDLL_HH
#define	KARABO_CORE_COREDLL_HH

/********************************************
 *         Disable Specific Warnings         *
 ********************************************/



/********************************************
 *            Windows DLL stuff             *  
 ********************************************/

#if defined(CORE_DLL)
#if defined(_WIN32)
#define DECLSPEC_CORE __declspec(dllexport)
#define TEMPLATE_CORE
#else
#define DECLSPEC_CORE
#define TEMPLATE_CORE
#endif
#else
#if defined(_WIN32)
#define DECLSPEC_CORE __declspec(dllimport)
#define TEMPLATE_CORE extern
#else
#define DECLSPEC_CORE
#define TEMPLATE_CORE extern
#endif
#endif

#endif
