/*
 * $Id
 *
 * Author: <kerstin.weger@xfel.eu>
 *
 * Created on March 19, 2012, 11:13 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTILTEST_UTILTESTDLL_HH
#define	KARABO_UTILTEST_UTILTESTDLL_HH

/********************************************
 *         Disable Specific Warnings         *
 ********************************************/



/********************************************
 *            Windows DLL stuff             *  
 ********************************************/

#if defined(UTILTEST_DLL)
#if defined(_WIN32)
#define DECLSPEC_UTILTEST __declspec(dllexport)
#define TEMPLATE_UTILTEST
#else
#define DECLSPEC_UTILTEST
#define TEMPLATE_UTILTEST
#endif
#else
#if defined(_WIN32)
#define DECLSPEC_UTILTEST __declspec(dllimport)
#define TEMPLATE_UTILTEST extern
#else
#define DECLSPEC_UTILTEST
#define TEMPLATE_UTILTEST extern
#endif
#endif

#endif
