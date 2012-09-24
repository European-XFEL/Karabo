/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 04, 2012, 11:13 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_XMS_XMSDLL_HH
#define	EXFEL_XMS_XMSDLL_HH

/********************************************
 *         Disable Specific Warnings         *
 ********************************************/
    

   
/********************************************
 *            Windows DLL stuff             *  
 ********************************************/

    #if defined(XMS_DLL)
        #if defined(_WIN32)
            #define DECLSPEC_XMS __declspec(dllexport)
            #define TEMPLATE_XMS
        #else
            #define DECLSPEC_XMS
            #define TEMPLATE_XMS
        #endif
    #else
        #if defined(_WIN32)
            #define DECLSPEC_XMS __declspec(dllimport)
            #define TEMPLATE_XMS extern
        #else
            #define DECLSPEC_XMS
            #define TEMPLATE_XMS extern
        #endif
    #endif

#endif
