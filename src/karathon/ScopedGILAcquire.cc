#include <iostream>
#include "ScopedGILAcquire.hh"

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
        PyObject* moduleSys = NULL;
        PyObject* moduleTraceback  = NULL;
        PyObject* functionFormatException = NULL;
        PyObject* obResult = NULL;
        PyObject* ptype = NULL;
        PyObject* pvalue = NULL;
        PyObject* ptraceback = NULL;
        std::string result = "";

        // Fetch parameters of error indicator ... the error indicator is getting cleared!
        // ... the new references returned!
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);

        // 'import sys' module ... otherwise 'import traceback' will fail (?!)
        moduleSys = PyImport_ImportModule("sys");
        if (!moduleSys) {
            std::cerr << "*** ERROR *** getPyErrString : 'import sys' failed\n" << std::endl;
            goto finish;
        }
        // 'import traceback' module ...
        moduleTraceback = PyImport_ImportModule("traceback");
        if (!moduleTraceback) {
            std::cerr << "*** ERROR *** getPyErrString : 'import traceback' failed\n" << std::endl;
            goto finish;
        }
        // get attribute 'format_exception'
        functionFormatException = PyObject_GetAttrString(moduleTraceback, "format_exception");
        if (!functionFormatException) {
            std::cerr << "*** ERROR *** getPyErrString : getattr(traceback,'format_exception') failed\n" << std::endl;
            goto finish;
        }

        if (ptraceback) {
            // call 'traceback.format_exception' with variable arguments API ...
            obResult = PyObject_CallFunctionObjArgs(functionFormatException, ptype, pvalue, ptraceback, NULL);
            if (!obResult) {
                std::cerr << "*** ERROR *** getPyErrString : traceback.format_exception(etype,evalue,etraceback) failed\n" << std::endl;
                goto finish;
            }
            Py_ssize_t length = PySequence_Length(obResult);
            for (Py_ssize_t i = 0; i < length; ++i) {
                PyObject* item = PySequence_GetItem(obResult, i);
                if (!item) continue;
                result += std::string(PyUnicode_AsUTF8(item));
                Py_DECREF(item);
            }
        } else if (pvalue) {
            PyObject* pystr = PyObject_Str(pvalue);
            if (pystr) {
                result = std::string(PyUnicode_AsUTF8(pystr));
                Py_DECREF(pystr);
            }
        }

    finish:

        Py_XDECREF(moduleSys);
        Py_XDECREF(moduleTraceback);
        Py_XDECREF(functionFormatException);
        Py_XDECREF(obResult);
        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);
        Py_XDECREF(ptraceback);

        return result;
    }
}

