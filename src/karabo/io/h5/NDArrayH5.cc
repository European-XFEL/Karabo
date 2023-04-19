/*
 * $Id$
 *
 * Author: <steffen.hauf@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#include "NDArrayH5.hh"


KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::CharNDArrayH5Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::Int8NDArrayH5Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::Int16NDArrayH5Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::Int32NDArrayH5Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::Int64NDArrayH5Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::UInt8NDArrayH5Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::UInt16NDArrayH5Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::UInt32NDArrayH5Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::UInt64NDArrayH5Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::BoolNDArrayH5Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::StringNDArrayH5Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::FloatNDArrayH5Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::DoubleNDArrayH5Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::ComplexFloatNDArrayH5Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::ComplexDoubleNDArrayH5Element)
