/*
 * File:   Wrapper.hh
 * Author: CONTROLS DEV group
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
