/*
 * $Id: TextFileOutput.cc 4951 2012-01-06 12:54:57Z heisenb@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 16, 2010, 10:18 PM
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

#include "TextFileOutput.hh"

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<karabo::data::Hash>,
                                  karabo::io::TextFileOutput<karabo::data::Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<karabo::data::Schema>,
                                  karabo::io::TextFileOutput<karabo::data::Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<std::vector<char> >,
                                  karabo::io::TextFileOutput<std::vector<char> >)
