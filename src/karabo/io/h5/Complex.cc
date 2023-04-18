/*
 * $Id: Complex.cc 9537 2013-04-26 07:36:59Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#include "Complex.hh"

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset, karabo::io::h5::FloatComplexElement)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Element, karabo::io::h5::Dataset,
                                  karabo::io::h5::DoubleComplexElement)
