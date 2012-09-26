/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <karabo/util/ImageElement.hh>
#include "PythonMacros.hh"

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;

void exportImageElement() {
    //karabo::util::ImageElement<int> , In Python: INT32_IMAGE_ELEMENT
    KARABO_PYTHON_IMAGE_ELEMENT(int, INT32)

    //karabo::util::ImageElement<double> , In Python: DOUBLE_IMAGE_ELEMENT       
    KARABO_PYTHON_IMAGE_ELEMENT(double, DOUBLE)
}
