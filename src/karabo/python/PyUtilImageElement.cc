/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <exfel/util/ImageElement.hh>
#include "PythonMacros.hh"

namespace bp = boost::python;
using namespace exfel::util;
using namespace std;

void exportImageElement() {
    //exfel::util::ImageElement<int> , In Python: INT32_IMAGE_ELEMENT
    EXFEL_PYTHON_IMAGE_ELEMENT(int, INT32)

    //exfel::util::ImageElement<int> , In Python: DOUBLE_IMAGE_ELEMENT       
    EXFEL_PYTHON_IMAGE_ELEMENT(double, DOUBLE)
}
