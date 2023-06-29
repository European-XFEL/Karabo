/*
 * File:   HandlerWrap.hh
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

#ifndef KARABIND_HANDLERWRAP_HH
#define KARABIND_HANDLERWRAP_HH

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "Wrapper.hh"


namespace py = pybind11;

namespace karabind {

    template <typename... Args> // if needed, could specify return type of operator() as fixed first template argument
    class HandlerWrap {
       public:
        /**
         * Construct a wrapper for a Python handler
         * @param handler the Python callable to wrap
         * @param where a C string identifying which handler is wrapped,
         *              for debugging only, i.e. used if a call to the handler raises an exception
         */
        HandlerWrap(const py::object& handler, char const* const where)
            : m_handler(std::make_shared<py::object>(handler)), // new object on the heap to control its destruction
              m_where(where) {}


        ~HandlerWrap() {
            // Ensure that destructor of Python handler object is called with GIL
            py::gil_scoped_acquire gil;
            m_handler.reset();
        }


        void operator()(Args... args) const {
            py::gil_scoped_acquire gil;
            try {
                if (*m_handler) {
                    // Just call handler with individually unpacked arguments:
                    (*m_handler)(py::cast(args)...); // std::forward(args)?
                }
            } catch (py::error_already_set& e) {
                detail::treatError_already_set(e, *m_handler, m_where);
            } catch (...) {
                KARABO_RETHROW
            }
        }

       protected: // not private - needed by HandlerWrapAny<N> and InputChannelWrap::DataHandlerWrap
        std::shared_ptr<py::object> m_handler;
        char const* const m_where;
    };

    /**
     * Specialisation of HandlerWrap for one boost::any argument
     *
     * The argument is converted to pybind11::object before passed to the Python handler.
     */
    class HandlerWrapAny1 : public HandlerWrap<const boost::any&> {
       public:
        HandlerWrapAny1(const py::object& handler, char const* const where)
            : HandlerWrap<const boost::any&>(handler, where) {}

        void operator()(const boost::any& a1) const;
    };

    /**
     * Specialisation of HandlerWrap for two boost::any arguments
     *
     * The arguments are converted to pybind11::object before passed to the Python handler.
     */
    class HandlerWrapAny2 : public HandlerWrap<const boost::any&, const boost::any&> {
       public:
        HandlerWrapAny2(const py::object& handler, char const* const where)
            : HandlerWrap<const boost::any&, const boost::any&>(handler, where) {}

        void operator()(const boost::any& a1, const boost::any& a2) const;
    };

    /**
     * Specialisation of HandlerWrap for three boost::any arguments
     *
     * The arguments are converted to pybind11::object before passed to the Python handler.
     */
    class HandlerWrapAny3 : public HandlerWrap<const boost::any&, const boost::any&, const boost::any&> {
       public:
        HandlerWrapAny3(const py::object& handler, char const* const where)
            : HandlerWrap<const boost::any&, const boost::any&, const boost::any&>(handler, where) {}

        void operator()(const boost::any& a1, const boost::any& a2, const boost::any& a3) const;
    };

    /**
     * Specialisation of HandlerWrap for four boost::any arguments
     *
     * The arguments are converted to pybind11::object before passed to the Python handler.
     */
    class HandlerWrapAny4
        : public HandlerWrap<const boost::any&, const boost::any&, const boost::any&, const boost::any&> {
       public:
        HandlerWrapAny4(const py::object& handler, char const* const where)
            : HandlerWrap<const boost::any&, const boost::any&, const boost::any&, const boost::any&>(handler, where) {}

        void operator()(const boost::any& a1, const boost::any& a2, const boost::any& a3, const boost::any& a4) const;
    };

    /**
     * Specialisation of HandlerWrap for two vector<unsigned long long> arguments
     *
     * The arguments are converted to pybind11::object before passed to the Python handler.
     */
    class HandlerWrapVullVull
        : public HandlerWrap<const std::vector<unsigned long long>&, const std::vector<unsigned long long>&> {
       public:
        HandlerWrapVullVull(const py::object& handler, char const* const where)
            : HandlerWrap<const std::vector<unsigned long long>&, const std::vector<unsigned long long>&>(handler,
                                                                                                          where) {}

        void operator()(const std::vector<unsigned long long>& v1, const std::vector<unsigned long long>& v2) const;
    };

} // namespace karabind

#endif /* KARABIND_HANDLERWRAP_HH */
