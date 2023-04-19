/*
 * $Id: TextFileReader.cc 4972 2012-01-13 10:17:15Z wegerk@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 16, 2011, 8:49 PM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "TextFileInput.hh"

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<karabo::util::Hash>, karabo::io::TextFileInput<karabo::util::Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<karabo::util::Schema>,
                                  karabo::io::TextFileInput<karabo::util::Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<std::vector<char> >, karabo::io::TextFileInput<std::vector<char> >)
