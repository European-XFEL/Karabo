/*
 * $Id: BinaryFileReader.cc 4972 2012-01-13 10:17:15Z wegerk@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 16, 2011, 8:49 PM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "BinaryFileInput.hh"

using namespace karabo::util;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<Hash>, karabo::io::BinaryFileInput<Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<Schema>, karabo::io::BinaryFileInput<Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<std::vector<char> >,
                                  karabo::io::BinaryFileInput<std::vector<char> >)
