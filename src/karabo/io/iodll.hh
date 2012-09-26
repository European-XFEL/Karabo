/*
 * $Id: iodll.hh 5395 2012-03-07 16:10:07Z wegerk $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 04, 2012, 16:13 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_IO_IODLL_HH
#define	KARABO_IO_IODLL_HH

/********************************************
 *         Disable Specific Warnings         *
 ********************************************/
    
  
/********************************************
 *            Windows DLL stuff             *  
 ********************************************/

    #if defined(IO_DLL)
        #if defined(_WIN32)
            #define DECLSPEC_IO __declspec(dllexport)
            #define TEMPLATE_IO
        #else
            #define DECLSPEC_IO
            #define TEMPLATE_IO
        #endif
    #else
        #if defined(_WIN32)
            #define DECLSPEC_IO __declspec(dllimport)
            #define TEMPLATE_IO extern
        #else
            #define DECLSPEC_IO
            #define TEMPLATE_IO extern
        #endif
    #endif

#endif
