/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 30, 2011, 2:51 PM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "Statics.hh"

namespace karabo {
    namespace xms {

        boost::uuids::random_generator Statics::m_uuidGenerator;

        unsigned int Statics::m_serverPorts = 0;

    } // namespace xms
} // namespace karabo
