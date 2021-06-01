#include <Python.h>

#include "SlotWrap.hh"
#include "ScopedGILAcquire.hh"

namespace bp = boost::python;

namespace karathon {


    SlotWrap::SlotWrap(const std::string& slotFunction) : karabo::xms::Slot(slotFunction) {
    }


    SlotWrap::~SlotWrap() {
        ScopedGILAcquire gil; // for bp:object destructor
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
            size_t numSelfArgs = 0ul;

            PyObject* function_object = NULL;
            // We expect either the function or the method to be given...
            if (PyFunction_Check(m_slotFunction->ptr())) {
                function_object = m_slotFunction->ptr();
            } else if (PyMethod_Check(m_slotFunction->ptr())) {
                function_object = PyMethod_Function(m_slotFunction->ptr());
                numSelfArgs = 1;
            } else {
                throw KARABO_PARAMETER_EXCEPTION("The argument is neither function nor method, please specify number of arguments.");
            }
            PyCodeObject* pycode = reinterpret_cast<PyCodeObject*> (PyFunction_GetCode(function_object));
            if (pycode) {
                // Note: co_argcount includes arguments with defaults, see nice figure from 'Hzzkygcs' at
                // https://stackoverflow.com/questions/847936/how-can-i-find-the-number-of-arguments-of-a-python-function
                m_arity = pycode->co_argcount - numSelfArgs; // Subtract "self" if any
            } else { // Can we get here?
                throw KARABO_PARAMETER_EXCEPTION("Failed to access PyCode object to deduce number of arguments.");
            }
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
            (*m_slotFunction)();
        } catch (const bp::error_already_set&) {
            rethrowPythonException();
        }
    }


    bp::object SlotWrap::getBodyArgument(const karabo::util::Hash& body, const char* key) const {
        // Avoid using HashWrap::get: it returns None if key missing (but we want the exception!).
        // Since get uses internally this getRef with the same const_cast, that is safe here as well
        return HashWrap::getRef(const_cast<karabo::util::Hash&> (body), bp::object(key));
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
        PyObject *e, *v, *t;
        // get the error indicators
        PyErr_Fetch(&e, &v, &t); // ref count incremented
        PyErr_NormalizeException(&e, &v, &t);
        
        std::string pythonErrorMessage;

        // Try to extract full traceback
        PyObject* moduleName = PyUnicode_FromString("traceback");
        PyObject* pythonModule = PyImport_Import(moduleName);
        Py_DECREF(moduleName);

        if (pythonModule != 0) {
            PyObject* pythonFunction = PyObject_GetAttrString(pythonModule, "format_exception");
            if (pythonFunction && PyCallable_Check(pythonFunction)) {
                PyObject* pythonValue = PyObject_CallFunctionObjArgs(pythonFunction, e, v, t, 0);
                if (pythonValue) {
                    if (PyList_Check(pythonValue)) {
                        pythonErrorMessage.append("Python reports ...\n\n");
                        Py_ssize_t size = PyList_Size(pythonValue);
                        for (Py_ssize_t i = 0; i < size; i++) {
                            PyObject* item = PyList_GetItem(pythonValue, i);  // this "borrowed reference" - no decref!
                            PyObject* pythonString = PyObject_Str(item);
                            pythonErrorMessage.append(PyUnicode_AsUTF8(pythonString));
                            Py_DECREF(pythonString);
                        }
                    }
                    Py_DECREF(pythonValue);
                }
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
