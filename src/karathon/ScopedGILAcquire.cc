#include "ScopedGILAcquire.hh"

#include <iostream>

namespace karathon {

    /**
     * Here we are implementing C API version of what is similar to the following Python code
     *
     * exc = ""
     * try:
     *    ... <Python code that can raise exception...>
     * except:
     *     exc_type, exc_value, exc_traceback = sys.exc_info()
     *     import traceback
     *     exc = traceback.format_exception(exc_type, exc_value, exc_traceback)
     */
    std::string getPythonExceptionAsString() {
        // Fetch parameters of error indicator ... the error indicator is getting cleared!
        // ... the new references returned!
        PyObject *ptype, *pvalue, *ptraceback;

        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);

        std::string result = "";
        PyObject *pystr = NULL, *separator = NULL, *moduleTraceback = NULL, *list = NULL;

        if (ptype) {
            if (pvalue && ptraceback) {
                separator = PyUnicode_FromString("\n");
                moduleTraceback = PyImport_ImportModule("traceback");
                // Letter "O" in format string denotes conversion from Object ... 3 arguments
                list = PyObject_CallMethod(moduleTraceback, "format_exception", "OOO", ptype, pvalue, ptraceback);
                if (list) {
                    pystr = PyUnicode_Join(separator, list);
                } else {
                    pystr = NULL;
                }
            }
            if (pvalue && !pystr) {
                pystr = PyObject_Str(pvalue);
            }
            if (pystr) {
                result = std::string(PyUnicode_AsUTF8(pystr));
            }
        }

        Py_XDECREF(moduleTraceback);
        Py_XDECREF(separator);
        Py_XDECREF(list);
        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);
        Py_XDECREF(ptraceback);
        Py_XDECREF(pystr);

        return result;
    }
} // namespace karathon
