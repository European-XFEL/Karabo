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
    //karabo::util::ImageElement , In Python: IMAGE_ELEMENT
    KARABO_PYTHON_IMAGE_ELEMENT

}
