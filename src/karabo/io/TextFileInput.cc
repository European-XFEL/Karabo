/*
 * $Id: TextFileReader.cc 4972 2012-01-13 10:17:15Z wegerk@DESY.DE $
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

#include "TextFileInput.hh"

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<karabo::data::Hash>, karabo::io::TextFileInput<karabo::data::Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<karabo::data::Schema>,
                                  karabo::io::TextFileInput<karabo::data::Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<std::vector<char> >, karabo::io::TextFileInput<std::vector<char> >)
