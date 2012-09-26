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

namespace karabo {
    namespace io {
        
        KARABO_REGISTER_FACTORY_CC(BinarySerializer<karabo::util::Hash>, HashBinarySerializer)
                
        HashBinarySerializer::HashBinarySerializer() {
            m_hashFormat = Format<karabo::util::Hash>::create("Bin");
        }


    }
}
