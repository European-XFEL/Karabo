/*
 * File:   Wrapper.hh
 * Author: CONTROLS DEV group
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef KARABIND_WRAPPER_HH
#define KARABIND_WRAPPER_HH

#include <pybind11/pybind11.h>

#include <boost/any.hpp>
#include <karabo/util/Hash.hh>
#include <karabo/util/ToLiteral.hh>
#include <karabo/util/Types.hh>

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(std::vector<karabo::util::Hash>);


namespace karabind {
    namespace wrapper {

        karabo::util::Types::ReferenceType pyObjectToCppType(const py::object& otype);

        py::object castAnyToPy(const boost::any& operand);

        karabo::util::Types::ReferenceType castPyToAny(const py::object& operand, boost::any& a);

        namespace detail {

            py::object castElementToPy(const karabo::util::Hash::Attributes::Node& self,
                                       const karabo::util::Types::ReferenceType& type);

            py::object castElementToPy(const karabo::util::Hash::Node& self,
                                       const karabo::util::Types::ReferenceType& type);
        } // namespace detail

    } // namespace wrapper
} // namespace karabind

#endif /* KARABIND_WRAPPER_HH */
