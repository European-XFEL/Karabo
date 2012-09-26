/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 04, 2012, 11:13 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_LOG_LOGDLL_HH
#define	KARABO_LOG_LOGDLL_HH

/********************************************
 *         Disable Specific Warnings         *
 ********************************************/
    

   
/********************************************
 *            Windows DLL stuff             *  
 ********************************************/

    #if defined(LOG_DLL)
        #if defined(_WIN32)
            #define DECLSPEC_LOG __declspec(dllexport)
            #define TEMPLATE_LOG
        #else
            #define DECLSPEC_LOG
            #define TEMPLATE_LOG
        #endif
    #else
        #if defined(_WIN32)
            #define DECLSPEC_LOG __declspec(dllimport)
            #define TEMPLATE_LOG extern
        #else
            #define DECLSPEC_LOG
            #define TEMPLATE_LOG extern
        #endif
    #endif

#endif
