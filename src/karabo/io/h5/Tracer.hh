/*
 * $Id: Format.cc 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */



#ifdef TRACER 
#define trace( level )\
 if ( TRACER >= level ) std::clog
#else 
#define trace( level) if(1); else std::clog
#endif



