/*
 * $Id: TextFileReader.cc 4972 2012-01-13 10:17:15Z wegerk@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 16, 2011, 8:49 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "TextFileInput.hh"

using namespace karabo::util;

namespace karabo {
    namespace io {

        KARABO_REGISTER_FOR_CONFIGURATION(Input<Hash>, TextFileInput<Hash> )
        KARABO_REGISTER_FOR_CONFIGURATION(Input<Schema>, TextFileInput<Schema> )
        KARABO_REGISTER_FOR_CONFIGURATION(Input<std::vector<char> >, TextFileInput<std::vector<char> > )
                
    } 
} 
