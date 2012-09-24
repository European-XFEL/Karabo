/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 11, 2012, 10:03 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "HashBinarySerializer.hh"
#include "Format.hh"

namespace exfel {
    namespace io {
        
        EXFEL_REGISTER_FACTORY_CC(BinarySerializer<exfel::util::Hash>, HashBinarySerializer)
                
        HashBinarySerializer::HashBinarySerializer() {
            m_hashFormat = Format<exfel::util::Hash>::create("Bin");
        }


    }
}
