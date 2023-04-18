/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#include "FixedLengthArray.hh"
using namespace karabo::io;


KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::CharArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::Int8ArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::Int16ArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::Int32ArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::Int64ArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::UInt8ArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::UInt16ArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::UInt32ArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::UInt64ArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::BoolArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::StringArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::FloatArrayElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::DoubleArrayElement)
