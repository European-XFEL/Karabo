/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 07, 2013, 10:18 AM
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

#include "BinaryFileOutput.hh"

using namespace karabo::data;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<Hash>, karabo::io::BinaryFileOutput<Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<Schema>, karabo::io::BinaryFileOutput<Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<std::vector<char> >,
                                  karabo::io::BinaryFileOutput<std::vector<char> >)
