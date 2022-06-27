/**
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <karabo/log/Logger.hh>
#include <karabo/util/Exception.hh>
#include <string>

namespace bp = boost::python;

// Translate C++ karabo::util::Exception into python RuntimeError exception
void translatorException(const karabo::util::Exception &e) {
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

void translatorRemoteException(const karabo::util::RemoteException &e) {
    // Assemble message from both, friendly message and details with "\nDETAILS:" as separator
    std::string msg(e.type());
    msg += ": ";
    msg += e.userFriendlyMsg(true); // Clear stack - details() below is not using it (and it should be empty anyway...)
    msg += "\nDETAILS: ";
    msg += e.type(); // Contains instanceId where exception occurred, so repeat also in details
    msg += ": ";
    msg += e.details(); // See ErrorHandlerWrap::operator() concerning details() vs detailedMsg()

    // Pass C-pointer to Python
    PyErr_SetString(PyExc_RuntimeError, msg.c_str());
}

void translatorTimeoutException(const karabo::util::TimeoutException &e) {
    const std::string msg(e.userFriendlyMsg(true)); // Clear stack

    // Pass C-pointer to Python
    PyErr_SetString(PyExc_TimeoutError, msg.c_str());
}

void exportPyUtilException() {
    // Register exception translators - order seems relevant:
    // First the base (to catch all not specifically mentioned), then the concrete ones.
    bp::register_exception_translator<karabo::util::Exception>(translatorException);
    bp::register_exception_translator<karabo::util::RemoteException>(translatorRemoteException);
    bp::register_exception_translator<karabo::util::TimeoutException>(translatorTimeoutException);
    // One could translate ParameterException to Python KeyError, but that does not well match:
    // ParameterException is also used when other input than keys are bad.
}