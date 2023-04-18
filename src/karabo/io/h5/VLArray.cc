/*
 * $Id: VLArray.cc 9537 2013-04-26 07:36:59Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#include "VLArray.hh"


KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::Int8VLArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::Int16VLArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::Int32VLArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::Int64VLArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::UInt8VLArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::UInt16VLArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::UInt32VLArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::UInt64VLArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::FloatVLArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::DoubleVLArrayElement)
