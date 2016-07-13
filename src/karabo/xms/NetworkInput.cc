/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "NetworkInput.hh"

using namespace karabo::util;
using namespace karabo::io;

namespace karabo {
    namespace xms {

        KARABO_REGISTER_FOR_CONFIGURATION(AbstractInput, Input<Hash >, NetworkInput<Hash>)
        KARABO_REGISTER_FOR_CONFIGURATION(Input<Hash >, NetworkInput<Hash>)

        KARABO_REGISTER_FOR_CONFIGURATION(AbstractInput, Input<std::vector<char> >, NetworkInput<std::vector<char> >)
        KARABO_REGISTER_FOR_CONFIGURATION(Input<std::vector<char> >, NetworkInput<std::vector<char> >)
    }
}
