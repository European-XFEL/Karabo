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

#include <karabo/xms/InputChannel.hh>

#include "Wrapper.hh"


namespace py = pybind11;

namespace karabind {
    template <typename Ret, typename... Args>
    class ReturnHandlerWrap {
       public:
        /**
         * Construct a wrapper for a Python handler whose return value is of interest.
         *
         * The return value of the handler must be castable to 'Ret',
         * 'Args` are the C++ arguments that the wrapper is expected to be called with.
         *
         * @param handler the Python callable to wrap
         * @param where a C string identifying which handler is wrapped,
         *              for debugging only, i.e. used if a call to the handler raises an exception
         */
        ReturnHandlerWrap(const py::object& handler, char const* const where)
            : m_handler(std::make_shared<py::object>(handler)), // new object on the heap to control its destruction
              m_where(where) {}


        ~ReturnHandlerWrap() {
            // Ensure that destructor of Python handler object is called with GIL
            py::gil_scoped_acquire gil;
            m_handler.reset();
        }


        Ret operator()(Args... args) const {
            py::gil_scoped_acquire gil;
            py::object pyResult;
            try {
                if (*m_handler) {
                    // Just call handler with individually unpacked arguments
                    pyResult = (*m_handler)(py::cast(std::forward<Args>(args))...);
                }
            } catch (py::error_already_set& e) {
                detail::treatError_already_set(e, *m_handler, m_where);
            } catch (...) {
                KARABO_RETHROW
            }
            return pyResult.cast<Ret>();
        }

       private: // may become protected if a derived class needs to overwrite operator()
        std::shared_ptr<py::object> m_handler;
        char const* const m_where;
    };

    template <typename... Args>
    class HandlerWrap {
       public:
        /**
         * Construct a wrapper for a Python handler whose return value is ignored.
         *
         * `Args` are the C++ arguments that the wrapper is expected to be called with.
         *
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
                    (*m_handler)(py::cast(std::forward<Args>(args))...);
                }
            } catch (py::error_already_set& e) {
                detail::treatError_already_set(e, *m_handler, m_where);
            } catch (...) {
                KARABO_RETHROW
            }
        }

       protected: // not private - needed by HandlerWrapExtra, HandlerWrapAny<N> and InputChannelWrap::DataHandlerWrap
        std::shared_ptr<py::object> m_handler;
        char const* const m_where;
    };

    /**
     * Specialisation of HandlerWrap that stores an extra C++ object and
     * passes it as second argument to the Python handler
     *
     * The arguments are converted to pybind11::object before passed to the Python handler.
     */
    template <typename FirstArg, typename ExtraArg, typename... Args>
    class HandlerWrapExtra : public HandlerWrap<FirstArg, Args...> {
       public:
        HandlerWrapExtra(const py::object& handler, char const* const where, const ExtraArg& extra)
            : HandlerWrap<FirstArg, Args...>(handler, where), m_extraArg(extra) {}

        void operator()(FirstArg first, Args... args) const {
            py::gil_scoped_acquire gil;
            try {
                if (*m_handler) {
                    // Call handler with individually unpacked arguments, but put the extra one as second:
                    (*m_handler)(py::cast(std::move(first)), py::cast(m_extraArg),
                                 py::cast(std::forward<Args>(args))...);
                }
            } catch (py::error_already_set& e) {
                detail::treatError_already_set(e, *m_handler, m_where);
            } catch (...) {
                KARABO_RETHROW
            }
        }

       private:
        ExtraArg m_extraArg;
        // Make (non-private) base class members available. Needed since 'C++ doesn’t consider superclass templates for
        // name resolution' (see
        // https://stackoverflow.com/questions/4010281/accessing-protected-members-of-superclass-in-c-with-templates):
        using HandlerWrap<FirstArg, Args...>::m_handler;
        using HandlerWrap<FirstArg, Args...>::m_where;
    };

    /**
     * Specialisation of HandlerWrap for one std::any argument
     *
     * The argument is converted to pybind11::object before passed to the Python handler.
     */
    class HandlerWrapAny1 : public HandlerWrap<const std::any&> {
       public:
        HandlerWrapAny1(const py::object& handler, char const* const where)
            : HandlerWrap<const std::any&>(handler, where) {}

        void operator()(const std::any& a1) const;
    };

    /**
     * Specialisation of HandlerWrap for two std::any arguments
     *
     * The arguments are converted to pybind11::object before passed to the Python handler.
     */
    class HandlerWrapAny2 : public HandlerWrap<const std::any&, const std::any&> {
       public:
        HandlerWrapAny2(const py::object& handler, char const* const where)
            : HandlerWrap<const std::any&, const std::any&>(handler, where) {}

        void operator()(const std::any& a1, const std::any& a2) const;
    };

    /**
     * Specialisation of HandlerWrap for three std::any arguments
     *
     * The arguments are converted to pybind11::object before passed to the Python handler.
     */
    class HandlerWrapAny3 : public HandlerWrap<const std::any&, const std::any&, const std::any&> {
       public:
        HandlerWrapAny3(const py::object& handler, char const* const where)
            : HandlerWrap<const std::any&, const std::any&, const std::any&>(handler, where) {}

        void operator()(const std::any& a1, const std::any& a2, const std::any& a3) const;
    };

    /**
     * Specialisation of HandlerWrap for four std::any arguments
     *
     * The arguments are converted to pybind11::object before passed to the Python handler.
     */
    class HandlerWrapAny4 : public HandlerWrap<const std::any&, const std::any&, const std::any&, const std::any&> {
       public:
        HandlerWrapAny4(const py::object& handler, char const* const where)
            : HandlerWrap<const std::any&, const std::any&, const std::any&, const std::any&>(handler, where) {}

        void operator()(const std::any& a1, const std::any& a2, const std::any& a3, const std::any& a4) const;
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


    class InputChannelDataHandler
        : public HandlerWrap<const karabo::data::Hash&, const karabo::xms::InputChannel::MetaData&> {
       public:
        InputChannelDataHandler(const py::object& handler, char const* const where)
            : HandlerWrap<const karabo::data::Hash&, const karabo::xms::InputChannel::MetaData&>(handler, where) {}

        void operator()(const karabo::data::Hash& data, const karabo::xms::InputChannel::MetaData& meta) const {
            py::gil_scoped_acquire gil;
            try {
                if (*m_handler) {
                    // TODO: wrap MetaData to expose full interface, right now this makes it look like a Hash within
                    // Python (Then one can get rid of InputChannelDataHandler and directly
                    // use HandlerWrap<const karabo::data::Hash&, const karabo::xms::InputChannel::MetaData&>)
                    (*m_handler)(data, *(reinterpret_cast<const karabo::data::Hash*>(&meta)));
                }
            } catch (py::error_already_set& e) {
                detail::treatError_already_set(e, *m_handler, m_where); // from Wrapper.hh
            } catch (...) {
                KARABO_RETHROW
            }
        }
    };
} // namespace karabind

#endif /* KARABIND_HANDLERWRAP_HH */
