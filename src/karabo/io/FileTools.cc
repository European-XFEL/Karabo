/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
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
