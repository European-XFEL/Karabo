/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 30, 2011, 2:51 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "Statics.hh"

namespace exfel {
    namespace xip {

        boost::uuids::random_generator Statics::m_uuidGenerator;
        
        unsigned int Statics::m_serverPorts = 0;

    }
}

