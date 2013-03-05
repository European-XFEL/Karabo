/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <karabo/util/TargetActualElement.hh>
#include "PythonMacros.hh"

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;

void exportTargetActualElement() {
    //karabo::util::TargetActualElement<int> , In Python: INT32_TARGETACTUAL_ELEMENT
    KARABO_PYTHON_TARGETACTUAL_ELEMENT(int, INT32)
            
    //karabo::util::TargetActualElement<unsigned int> , In Python: UINT32_TARGETACTUAL_ELEMENT
    KARABO_PYTHON_TARGETACTUAL_ELEMENT(unsigned int, UINT32)
            
    //karabo::util::TargetActualElement<long long> , In Python: INT64_TARGETACTUAL_ELEMENT
    KARABO_PYTHON_TARGETACTUAL_ELEMENT(long long, INT64)
            
    //karabo::util::TargetActualElement<unsigned long long> , In Python: UINT64_TARGETACTUAL_ELEMENT
    KARABO_PYTHON_TARGETACTUAL_ELEMENT(unsigned long long, UINT64)

    //karabo::util::TargetActualElement<double> , In Python: DOUBLE_TARGETACTUAL_ELEMENT       
    KARABO_PYTHON_TARGETACTUAL_ELEMENT(double, DOUBLE)
            
    //karabo::util::TargetActualElement<bool> , In Python: BOOL_TARGETACTUAL_ELEMENT       
    KARABO_PYTHON_TARGETACTUAL_ELEMENT(bool, BOOL)
            
    //karabo::util::TargetActualElement<string> , In Python: STRING_TARGETACTUAL_ELEMENT       
    KARABO_PYTHON_TARGETACTUAL_ELEMENT(string, STRING)
}
