#include "SlotWrap.hh"

namespace bp = boost::python;

namespace karathon {


    SlotWrap::SlotWrap(const std::string& slotFunction) : karabo::xms::Slot(slotFunction) {
    }


    SlotWrap::~SlotWrap() {
    }


    void SlotWrap::registerSlotFunction(const bp::object& slotHandler) {
        // We accept ONLY the python callable
        if (!PyCallable_Check(slotHandler.ptr()))
            throw KARABO_PARAMETER_EXCEPTION("The argument is not callable.  Function or method is expected.");

        m_slotFunction = slotHandler;
        m_arity = 0;

        PyObject* function_object = NULL;
        // We expect either the function or the method to be given...
        if (PyFunction_Check(m_slotFunction.ptr()))
            function_object = m_slotFunction.ptr();
        else if (PyMethod_Check(m_slotFunction.ptr())) {
            function_object = PyMethod_Function(m_slotFunction.ptr());
            m_arity = 1; // Argument self counted
        } else
            throw KARABO_PARAMETER_EXCEPTION("The argument is neither the function not the method.");
        PyCodeObject* pycode = reinterpret_cast<PyCodeObject*> (PyFunction_GetCode(function_object));
        if (pycode) m_arity = pycode->co_argcount - m_arity; // Subtract "self"
    }


    void SlotWrap::doCallRegisteredSlotFunctions(const karabo::util::Hash& body) {

        ScopedGILAcquire gil;

        switch (m_arity) {
            case 4:
                if (callFunction4(body)) return;
                break;
            case 3:
                if (callFunction3(body)) return;
                break;
            case 2:
                if (callFunction2(body)) return;
                break;
            case 1:
                if (callFunction1(body)) return;
                break;
            case 0:
                if (callFunction0(body)) return;
                break;
            default:
                throw KARABO_SIGNALSLOT_EXCEPTION("Too many arguments send to python slot (max 4 are currently supported");
        }
        throw KARABO_LOGIC_EXCEPTION("TypeError exception happened \"somewhere\" in Python code");
    }


    bool SlotWrap::callFunction0(const karabo::util::Hash& body) {
        try {
            m_slotFunction();
        } catch (const bp::error_already_set&) {
            PyErr_Print();
            return false;
        }
        return true;
    }


    bool SlotWrap::callFunction1(const karabo::util::Hash& body) {
        bp::object a1 = HashWrap::get(body, "a1");
        try {
            m_slotFunction(a1);
        } catch (const bp::error_already_set&) {
            tryToPrint();
            return false;
        }
        return true;
    }


    bool SlotWrap::callFunction2(const karabo::util::Hash& body) {
        bp::object a1 = HashWrap::get(body, "a1");
        bp::object a2 = HashWrap::get(body, "a2");
        try {
            m_slotFunction(a1, a2);
        } catch (const bp::error_already_set&) {
            tryToPrint();
            return false;
        }
        return true;
    }


    bool SlotWrap::callFunction3(const karabo::util::Hash& body) {
        bp::object a1 = HashWrap::get(body, "a1");
        bp::object a2 = HashWrap::get(body, "a2");
        bp::object a3 = HashWrap::get(body, "a3");
        try {
            m_slotFunction(a1, a2, a3);
        } catch (const bp::error_already_set&) {
            tryToPrint();
            return false;
        }
        return true;
    }


    bool SlotWrap::callFunction4(const karabo::util::Hash& body) {
        bp::object a1 = HashWrap::get(body, "a1");
        bp::object a2 = HashWrap::get(body, "a2");
        bp::object a3 = HashWrap::get(body, "a3");
        bp::object a4 = HashWrap::get(body, "a4");
        try {
            m_slotFunction(a1, a2, a3, a4);
        } catch (const bp::error_already_set&) {
            tryToPrint();
            return false;
        }
        return true;
    }


    void SlotWrap::tryToPrint() {
        PyObject *e, *v, *t;
        bool printing = true;

        // get the error indicators
        PyErr_Fetch(&e, &v, &t); // ref count incremented

        if (PyErr_GivenExceptionMatches(e, PyExc_TypeError))
            printing = false;

        // we reset it for later processing
        PyErr_Restore(e, v, t); // ref count decremented
        if (printing)
            PyErr_Print();
        else
            PyErr_Clear();
    }

}
