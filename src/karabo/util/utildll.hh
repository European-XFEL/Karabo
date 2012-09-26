/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 04, 2012, 11:13 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_UTILDLL_HH
#define	KARABO_UTIL_UTILDLL_HH

/********************************************
 *         Disable Specific Warnings         *
 ********************************************/
    

   
/********************************************
 *            Windows DLL stuff             *  
 ********************************************/

    #if defined(UTIL_DLL)
        #if defined(_WIN32)
            #define DECLSPEC_UTIL __declspec(dllexport)
            #define TEMPLATE_UTIL
        #else
            #define DECLSPEC_UTIL
            #define TEMPLATE_UTIL
        #endif
    #else
        #if defined(_WIN32)
            #define DECLSPEC_UTIL __declspec(dllimport)
            #define TEMPLATE_UTIL extern
        #else
            #define DECLSPEC_UTIL
            #define TEMPLATE_UTIL extern
        #endif
    #endif

#endif
