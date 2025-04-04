/**
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

#include <pybind11/pybind11.h>

#include <karabo/log/Logger.hh>
#include <string>

#include "karabo/data/types/Exception.hh"

namespace py = pybind11;


void exportPyUtilException(py::module_& m) {
    // Translate C++ karabo::data::Exception into python RuntimeError exception
    py::register_exception_translator([](std::exception_ptr p) {
        try {
            if (p) {
                std::rethrow_exception(p);
            }
        } catch (const karabo::data::RemoteException& e) {
            // Assemble message from both, friendly message and details with "\nDETAILS:" as separator
            std::string msg(e.type());
            msg += ": ";
            msg += e.userFriendlyMsg(
                  true); // Clear stack - details() below is not using it (and it should be empty anyway...)
            msg += "\nDETAILS: ";
            msg += e.type(); // Contains instanceId where exception occurred, so repeat also in details
            msg += ": ";
            msg += e.details(); // See ErrorHandlerWrap::operator() concerning details() vs detailedMsg()

            // Pass C-pointer to Python
            PyErr_SetString(PyExc_RuntimeError, msg.c_str());
        } catch (const karabo::data::TimeoutException& e) {
            const std::string msg(e.userFriendlyMsg(true)); // Clear stack

            // Pass C-pointer to Python
            PyErr_SetString(PyExc_TimeoutError, msg.c_str());
        } catch (const karabo::data::Exception& e) {
            // Assemble message from type, friendly message and - separated by \nDETAILS:\n" - detailed message.
            std::string msg(e.type());
            msg += ": ";
            msg += e.userFriendlyMsg(false);
            msg += "\nDETAILS:\n";
            // Order of calls to userFriendlyMsg(false) and detailedMsg() matters since the latter clears the stack.
            msg += e.detailedMsg();

            // Pass C-pointer to Python
            PyErr_SetString(PyExc_RuntimeError, msg.c_str());
        }
    });
}
