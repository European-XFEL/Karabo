#include <Python.h>
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
        m_varargs = false;
        m_varkeywords = false;

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
        if (pycode) {
            m_arity = pycode->co_argcount - m_arity; // Subtract "self" if any
            if (pycode->co_flags && CO_VARARGS) m_varargs = true;
            if (pycode->co_flags && CO_VARKEYWORDS) m_varkeywords = true;
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
                throw KARABO_SIGNALSLOT_EXCEPTION("Too many arguments send to python slot (max 4 are currently supported");
        }
    }


    void SlotWrap::callFunction0(const karabo::util::Hash& body) {
        try {
            m_slotFunction();
        } catch (const bp::error_already_set&) {
            rethrowPythonException();
        }
    }


    void SlotWrap::callFunction1(const karabo::util::Hash& body) {
        bp::object a1 = HashWrap::get(body, "a1");
        try {
            m_slotFunction(a1);
        } catch (const bp::error_already_set&) {
            rethrowPythonException();
        }
    }


    void SlotWrap::callFunction2(const karabo::util::Hash& body) {
        bp::object a1 = HashWrap::get(body, "a1");
        bp::object a2 = HashWrap::get(body, "a2");
        try {
            m_slotFunction(a1, a2);
        } catch (const bp::error_already_set&) {
            rethrowPythonException();
        }
    }


    void SlotWrap::callFunction3(const karabo::util::Hash& body) {
        bp::object a1 = HashWrap::get(body, "a1");
        bp::object a2 = HashWrap::get(body, "a2");
        bp::object a3 = HashWrap::get(body, "a3");
        try {
            m_slotFunction(a1, a2, a3);
        } catch (const bp::error_already_set&) {
            rethrowPythonException();
        }
    }


    void SlotWrap::callFunction4(const karabo::util::Hash& body) {
        bp::object a1 = HashWrap::get(body, "a1");
        bp::object a2 = HashWrap::get(body, "a2");
        bp::object a3 = HashWrap::get(body, "a3");
        bp::object a4 = HashWrap::get(body, "a4");
        try {
            m_slotFunction(a1, a2, a3, a4);
        } catch (const bp::error_already_set&) {
            rethrowPythonException();
        }
    }


    void SlotWrap::rethrowPythonException() {
        PyObject *e, *v, *t;
         // get the error indicators
        PyErr_Fetch(&e, &v, &t); // ref count incremented

        std::string pythonErrorMessage;

        // Try to extract full traceback
        PyObject* moduleName = PyUnicode_FromString("traceback");
        PyObject* pythonModule = PyImport_Import(moduleName);
        Py_DECREF(moduleName);
        
        if (pythonModule != 0) {
            PyObject* pythonFunction = PyObject_GetAttrString(pythonModule, "format_exception");
            if (pythonFunction && PyCallable_Check(pythonFunction)) {
                PyObject* pythonValue = PyObject_CallFunctionObjArgs(pythonFunction, e, v, t, 0);
                if (PyList_Check(pythonValue)) {
                    pythonErrorMessage.clear();
                    pythonErrorMessage.append("\n\n");
                    Py_ssize_t size = PyList_Size(pythonValue);
                    for (Py_ssize_t i = 0; i < size; i++) {
                        PyObject* item = PyList_GetItem(pythonValue, i);  // this "borrowed reference" - no decref!
                        PyObject* pythonString = PyObject_Str(item);
                        pythonErrorMessage.append(PyUnicode_AsUTF8(pythonString));
                        Py_DECREF(pythonString);
                    }
                }
                Py_DECREF(pythonValue);
                Py_DECREF(pythonFunction);
            }
            Py_DECREF(pythonModule);
        } else {
            PyObject* pythonRepr = PyObject_Repr(v);   // apply str()
            pythonErrorMessage.assign(PyUnicode_AsUTF8(pythonRepr));
            Py_DECREF(pythonRepr);
        }

        // we reset it for later processing
        PyErr_Restore(e, v, t); // ref count decremented
        PyErr_Clear();

        throw KARABO_PYTHON_EXCEPTION(pythonErrorMessage);
    }

}
