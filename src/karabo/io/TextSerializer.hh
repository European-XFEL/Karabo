/*
 * $Id$
 *
 * File:   TextSerializer.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 20, 2013, 11:05 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_IO_TEXTSERIALIZER_HH
#define	KARABO_IO_TEXTSERIALIZER_HH

#include <vector>

#include <karabo/util/Configurator.hh>

namespace karabo {
    namespace io {
        
        template <class T>
        class TextSerializer {
            
            public:
                
                KARABO_CLASSINFO(TextSerializer, "TextSerializer", "1.0")
                
                virtual void save(const T& object, std::string& archive) = 0;
                
                virtual void load(T& object, const std::string& archive) = 0;
                
        };
        
    }
}

#endif
