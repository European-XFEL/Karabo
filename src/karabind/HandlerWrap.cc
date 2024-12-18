/*
 * File:   HandlerWrap.cc
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

#include "HandlerWrap.hh"

namespace karabind {

    void HandlerWrapAny1::operator()(const std::any& a1) const {
        py::gil_scoped_acquire gil;
        try {
            if (*m_handler) {
                (*m_handler)(wrapper::castAnyToPy(a1));
            }
        } catch (py::error_already_set& e) {
            detail::treatError_already_set(e, *m_handler, m_where);
        } catch (...) {
            KARABO_RETHROW
        }
    }

    void HandlerWrapAny2::operator()(const std::any& a1, const std::any& a2) const {
        py::gil_scoped_acquire gil;
        try {
            if (*m_handler) {
                (*m_handler)(wrapper::castAnyToPy(a1), wrapper::castAnyToPy(a2));
            }
        } catch (py::error_already_set& e) {
            detail::treatError_already_set(e, *m_handler, m_where);
        } catch (...) {
            KARABO_RETHROW
        }
    }

    void HandlerWrapAny3::operator()(const std::any& a1, const std::any& a2, const std::any& a3) const {
        py::gil_scoped_acquire gil;
        try {
            if (*m_handler) {
                (*m_handler)(wrapper::castAnyToPy(a1), wrapper::castAnyToPy(a2), wrapper::castAnyToPy(a3));
            }
        } catch (py::error_already_set& e) {
            detail::treatError_already_set(e, *m_handler, m_where);
        } catch (...) {
            KARABO_RETHROW
        }
    }

    void HandlerWrapAny4::operator()(const std::any& a1, const std::any& a2, const std::any& a3,
                                     const std::any& a4) const {
        py::gil_scoped_acquire gil;
        try {
            if (*m_handler) {
                (*m_handler)(wrapper::castAnyToPy(a1), wrapper::castAnyToPy(a2), wrapper::castAnyToPy(a3),
                             wrapper::castAnyToPy(a4));
            }
        } catch (py::error_already_set& e) {
            detail::treatError_already_set(e, *m_handler, m_where);
        } catch (...) {
            KARABO_RETHROW
        }
    }

    void HandlerWrapVullVull::operator()(const std::vector<unsigned long long>& v1,
                                         const std::vector<unsigned long long>& v2) const {
        py::gil_scoped_acquire gil;
        try {
            if (*m_handler) {
                (*m_handler)(py::cast(v1), py::cast(v2));
            }
        } catch (py::error_already_set& e) {
            detail::treatError_already_set(e, *m_handler, m_where);
        } catch (...) {
            KARABO_RETHROW
        }
    }

} // namespace karabind
