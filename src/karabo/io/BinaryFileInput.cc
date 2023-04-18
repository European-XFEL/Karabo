/*
 * $Id: BinaryFileReader.cc 4972 2012-01-13 10:17:15Z wegerk@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 16, 2011, 8:49 PM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "BinaryFileInput.hh"

using namespace karabo::util;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<Hash>, karabo::io::BinaryFileInput<Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<Schema>, karabo::io::BinaryFileInput<Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<std::vector<char> >,
                                  karabo::io::BinaryFileInput<std::vector<char> >)
