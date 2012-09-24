/*
 * $Id
 *
 * Author: <kerstin.weger@xfel.eu>
 *
 * Created on March 19, 2012, 11:13 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_IOTEST_IOTESTDLL_HH
#define	EXFEL_IOTEST_IOTESTDLL_HH

/********************************************
 *         Disable Specific Warnings         *
 ********************************************/
    

   
/********************************************
 *            Windows DLL stuff             *  
 ********************************************/

    #if defined(IOTEST_DLL)
        #if defined(_WIN32)
            #define DECLSPEC_IOTEST __declspec(dllexport)
            #define TEMPLATE_IOTEST
        #else
            #define DECLSPEC_IOTEST
            #define TEMPLATE_IOTEST
        #endif
    #else
        #if defined(_WIN32)
            #define DECLSPEC_IOTEST __declspec(dllimport)
            #define TEMPLATE_IOTEST extern
        #else
            #define DECLSPEC_IOTEST
            #define TEMPLATE_IOTEST extern
        #endif
    #endif

#endif
