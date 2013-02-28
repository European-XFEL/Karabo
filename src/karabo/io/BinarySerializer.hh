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

#ifndef KARABO_IO_BINARYSERIALIZER_HH
#define	KARABO_IO_BINARYSERIALIZER_HH

#include <vector>

#include <karabo/util/Configurator.hh>

namespace karabo {
    namespace io {
        
        template <class T>
        class BinarySerializer {
            
            public:
                
                KARABO_CLASSINFO(BinarySerializer, "BinarySerializer", "1.0")
                KARABO_CONFIGURATION_BASE_CLASS;
                
                virtual void save(const T& object, std::vector<char>& archive) = 0;
                
                virtual void load(T& object, const char* archive, const size_t nBytes) = 0;
                    
                void load(T& object, const std::vector<char>& archive) {
                    load(object, &archive[0], archive.size());
                }
                
        };
        
    }
}

#endif
