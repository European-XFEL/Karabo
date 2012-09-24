/*
 * $Id$
 *
 * File:   BinarySerializer.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 10, 2012, 5:05 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_IO_BINARYSERIALIZER_HH
#define	EXFEL_IO_BINARYSERIALIZER_HH

#include <vector>

#include <karabo/util/Factory.hh>

namespace exfel {
    namespace io {
        
        template <class T>
        class BinarySerializer {
            
            public:
                
                EXFEL_CLASSINFO(BinarySerializer, "BinarySerializer", "1.0")
                
                EXFEL_FACTORY_BASE_CLASS
                
                virtual void save(const T& object, std::vector<char>& archive) = 0;
                
                virtual void load(T& object, const char* archive, const size_t nBytes) = 0;
                    
                void load(T& object, const std::vector<char>& archive) {
                    load(object, &archive[0], archive.size());
                }
                
        };
        
    }
}

#endif
