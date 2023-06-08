/*
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
#include "FileTools.hh"

template void karabo::io::loadFromFile<karabo::util::Hash>(karabo::util::Hash& object, const std::string& filename,
                                                           const karabo::util::Hash& config = karabo::util::Hash());
template void karabo::io::loadFromFile<karabo::util::Schema>(karabo::util::Schema& object, const std::string& filename,
                                                             const karabo::util::Hash& config = karabo::util::Hash());
template void karabo::io::saveToFile<karabo::util::Hash>(const karabo::util::Hash& object, const std::string& filename,
                                                         const karabo::util::Hash& config = karabo::util::Hash());
template void karabo::io::saveToFile<karabo::util::Schema>(const karabo::util::Schema& object,
                                                           const std::string& filename,
                                                           const karabo::util::Hash& config = karabo::util::Hash());
template std::string karabo::io::getIODataType<karabo::util::Hash>();
template std::string karabo::io::getIODataType<karabo::util::Schema>();
