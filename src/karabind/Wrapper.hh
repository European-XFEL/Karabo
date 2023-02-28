/*
 * Author: CONTROLS DEV group
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABIND_WRAPPER_HH
#define KARABIND_WRAPPER_HH

#include <pybind11/pybind11.h>
#include <boost/any.hpp>
#include <karabo/util/Types.hh>

namespace py = pybind11;

namespace karabind {
    namespace wrapper {

        py::object castAnyToPy(const boost::any& operand);
        karabo::util::Types::ReferenceType castPyToAny(const py::object& operand, boost::any& a);

    } // namespace wrapper
} // namespace karabind

#endif /* KARABIND_WRAPPER_HH */
