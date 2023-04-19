/*
 * $Id: TextFileOutput.cc 4951 2012-01-06 12:54:57Z heisenb@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 16, 2010, 10:18 PM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "TextFileOutput.hh"

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<karabo::util::Hash>,
                                  karabo::io::TextFileOutput<karabo::util::Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<karabo::util::Schema>,
                                  karabo::io::TextFileOutput<karabo::util::Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<std::vector<char> >,
                                  karabo::io::TextFileOutput<std::vector<char> >)
