/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
#include "SlotWrap.hh"

#include <Python.h>

#include "ScopedGILAcquire.hh"
#include "Wrapper.hh" // for getPythonExceptionStrings()

namespace bp = boost::python;

namespace karathon {


    SlotWrap::SlotWrap(const std::string& slotFunction) : karabo::xms::Slot(slotFunction) {}


    SlotWrap::~SlotWrap() {
        ScopedGILAcquire gil; // for bp::object destructor
        m_slotFunction.reset();
    }


    void SlotWrap::registerSlotFunction(const bp::object& slotHandler, int numArgs) {
        // We accept ONLY the python callable
        if (!PyCallable_Check(slotHandler.ptr())) {
            throw KARABO_PARAMETER_EXCEPTION("The argument is not callable.");
        }

        // Note: In C++ there is a list of slot handlers to which is appended - here we overwrite any previous handler.
        // Note 2: Create pointer to be able to ensure that deletion can be done with GIL acquired.
        std::unique_ptr<bp::object> newHandler(std::make_unique<bp::object>(slotHandler));
        m_slotFunction.swap(newHandler);
        if (numArgs >= 0) {
            m_arity = numArgs;
        } else {
            // Undefined number of arguments, try to figure out
            m_arity = Wrapper::numArgs(*m_slotFunction);
        }
    }


    void SlotWrap::doCallRegisteredSlotFunctions(const karabo::util::Hash& body) {
        ScopedGILAcquire gil;

        switch (m_arity) {
            case 4:
                callFunction4(body);
                break;
            case 3:
                callFunction3(body);
                break;
            case 2:
                callFunction2(body);
                break;
            case 1:
                callFunction1(body);
                break;
            case 0:
                callFunction0(body);
                break;
            default:
                throw KARABO_SIGNALSLOT_EXCEPTION(
                      "Too many arguments send to python slot (max 4 are currently supported");
        }
    }


    void SlotWrap::callFunction0(const karabo::util::Hash& body) {
        try {
            (*m_slotFunction)();
        } catch (const bp::error_already_set&) {
            rethrowPythonException();
        }
    }


    bp::object SlotWrap::getBodyArgument(const karabo::util::Hash& body, const char* key) const {
        // Avoid using HashWrap::get: it returns None if key missing (but we want the exception!).
        // Since get uses internally this getRef with the same const_cast, that is safe here as well
        return HashWrap::getRef(const_cast<karabo::util::Hash&>(body), bp::object(key));
    }


    void SlotWrap::callFunction1(const karabo::util::Hash& body) {
        bp::object a1 = getBodyArgument(body, "a1");
        try {
            (*m_slotFunction)(a1);
        } catch (const bp::error_already_set&) {
            rethrowPythonException();
        }
    }


    void SlotWrap::callFunction2(const karabo::util::Hash& body) {
        bp::object a1 = getBodyArgument(body, "a1");
        bp::object a2 = getBodyArgument(body, "a2");
        try {
            (*m_slotFunction)(a1, a2);
        } catch (const bp::error_already_set&) {
            rethrowPythonException();
        }
    }


    void SlotWrap::callFunction3(const karabo::util::Hash& body) {
        bp::object a1 = getBodyArgument(body, "a1");
        bp::object a2 = getBodyArgument(body, "a2");
        bp::object a3 = getBodyArgument(body, "a3");
        try {
            (*m_slotFunction)(a1, a2, a3);
        } catch (const bp::error_already_set&) {
            rethrowPythonException();
        }
    }


    void SlotWrap::callFunction4(const karabo::util::Hash& body) {
        bp::object a1 = getBodyArgument(body, "a1");
        bp::object a2 = getBodyArgument(body, "a2");
        bp::object a3 = getBodyArgument(body, "a3");
        bp::object a4 = getBodyArgument(body, "a4");
        try {
            (*m_slotFunction)(a1, a2, a3, a4);
        } catch (const bp::error_already_set&) {
            rethrowPythonException();
        }
    }


    void SlotWrap::rethrowPythonException() {
        std::string pythonErrorMessage, pythonErrorDetails;
        std::tie(pythonErrorMessage, pythonErrorDetails) = getPythonExceptionStrings();

        throw KARABO_PYTHON_EXCEPTION2(pythonErrorMessage, pythonErrorDetails);
    }

} // namespace karathon
