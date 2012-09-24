/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 04, 2012, 11:13 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_NET_NETDLL_HH
#define	EXFEL_NET_NETDLL_HH

/********************************************
 *         Disable Specific Warnings         *
 ********************************************/
    

   
/********************************************
 *            Windows DLL stuff             *  
 ********************************************/

    #if defined(NET_DLL)
        #if defined(_WIN32)
            #define DECLSPEC_NET __declspec(dllexport)
            #define TEMPLATE_NET
        #else
            #define DECLSPEC_NET
            #define TEMPLATE_NET
        #endif
    #else
        #if defined(_WIN32)
            #define DECLSPEC_NET __declspec(dllimport)
            #define TEMPLATE_NET extern
        #else
            #define DECLSPEC_NET
            #define TEMPLATE_NET extern
        #endif
    #endif

#endif
