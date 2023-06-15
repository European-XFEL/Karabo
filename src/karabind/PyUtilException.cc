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
#include <karabo/util/Exception.hh>
#include <string>

namespace py = pybind11;

void exportPyUtilException(py::module_& m) {
    // Register exception translator
    static py::exception<karabo::util::Exception> exc(m, "KaraboError", PyExc_RuntimeError);
    static py::exception<karabo::util::RemoteException> rexc(m, "KaraboRemoteError", PyExc_RuntimeError);
    static py::exception<karabo::util::TimeoutException> texc(m, "KaraboTimeoutError", PyExc_TimeoutError);

    py::register_exception_translator([](std::exception_ptr p) {
        try {
            if (p) {
                std::rethrow_exception(p);
            }
        } catch (const karabo::util::TimeoutException& te) {
            const std::string msg(te.userFriendlyMsg(true)); // Clear stack
            texc(msg.c_str());
        } catch (const karabo::util::RemoteException& re) {
            // Assemble message from both, friendly message and details with "\nDETAILS:" as separator
            std::string msg(re.type());
            msg += ": ";
            msg += re.userFriendlyMsg(
                  true); // Clear stack - details() below is not using it (and it should be empty anyway...)
            msg += "\nDETAILS: ";
            msg += re.type(); // Contains instanceId where exception occurred, so repeat also in details
            msg += ": ";
            msg += re.details(); // See ErrorHandlerWrap::operator() concerning details() vs detailedMsg()
            rexc(msg.c_str());
        } catch (const karabo::util::Exception& ee) {
            // Assemble message from type, friendly message and - separated by \nDETAILS:\n" - detailed message.
            // Set Exception as the active python error
            std::string msg(ee.type());
            msg += ": ";
            msg += ee.userFriendlyMsg(false);
            msg += "\nDETAILS:\n";
            // Order of calls to userFriendlyMsg(false) and detailedMsg() matters since the latter clears the stack.
            msg += ee.detailedMsg();
            exc(msg.c_str());
        }
    });

    // One could translate ParameterException to Python KeyError, but that does not well match:
    // ParameterException is also used when other input than keys are bad.
}
