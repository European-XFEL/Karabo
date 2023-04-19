/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 07, 2013, 10:18 AM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "BinaryFileOutput.hh"

using namespace karabo::util;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<Hash>, karabo::io::BinaryFileOutput<Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<Schema>, karabo::io::BinaryFileOutput<Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<std::vector<char> >,
                                  karabo::io::BinaryFileOutput<std::vector<char> >)
