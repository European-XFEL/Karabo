/*
 * $Id: TextFileOutput.cc 4951 2012-01-06 12:54:57Z heisenb@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 16, 2010, 10:18 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "TextFileOutput.hh"

using namespace karabo::util;

namespace karabo {
    namespace io {

        KARABO_REGISTER_FOR_CONFIGURATION(Output<Hash>, TextFileOutput<Hash>)
        KARABO_REGISTER_FOR_CONFIGURATION(Output<Schema>, TextFileOutput<Schema>)
        KARABO_REGISTER_FOR_CONFIGURATION(Output<std::vector<char> >, TextFileOutput<std::vector<char> >)
    }
}
