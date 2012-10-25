/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 30, 2011, 7:33 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_XIP_GPUIMAGE_CUH
#define	EXFEL_XIP_GPUIMAGE_CUH

namespace exfel {
  namespace xip {
        
    namespace ut = exfel::util;
    
    extern "C" void cudaFill(const ut::Types::Type type, void* pixels, int size, const void* value);
    
  }
}



#endif	

