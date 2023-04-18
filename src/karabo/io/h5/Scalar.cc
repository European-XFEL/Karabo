/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#include "Scalar.hh"
using namespace karabo::io::h5;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::CharElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::Int8Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::Int16Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::Int32Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::Int64Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::UInt8Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::UInt16Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::UInt32Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::UInt64Element)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::BoolElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::StringElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::FloatElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::DoubleElement)

//            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
//            karabo::io::h5::DatasetAttribute, karabo::io::h5::CharAttributeElement)
//            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
//            karabo::io::h5::DatasetAttribute, karabo::io::h5::Int8AttributeElement)
//            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
//            karabo::io::h5::DatasetAttribute, karabo::io::h5::Int16AttributeElement)
//            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
//            karabo::io::h5::DatasetAttribute, karabo::io::h5::Int32AttributeElement)
//            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
//            karabo::io::h5::DatasetAttribute, karabo::io::h5::Int64AttributeElement)
//            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
//            karabo::io::h5::DatasetAttribute, karabo::io::h5::UInt8AttributeElement)
//            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
//            karabo::io::h5::DatasetAttribute, karabo::io::h5::UInt16AttributeElement)
//            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
//            karabo::io::h5::DatasetAttribute, karabo::io::h5::UInt32AttributeElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::DatasetAttribute,
                                  karabo::io::h5::UInt64AttributeElement)
//            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
//            karabo::io::h5::DatasetAttribute, karabo::io::h5::BoolAttributeElement)
//            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
//            karabo::io::h5::DatasetAttribute, karabo::io::h5::StringAttributeElement)
//            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
//            karabo::io::h5::DatasetAttribute, karabo::io::h5::FloatAttributeElement)
//            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
//            karabo::io::h5::DatasetAttribute, karabo::io::h5::DoubleAttributeElement)
