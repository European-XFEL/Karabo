#include <iostream>
#include "ScopedGILAcquire.hh"

namespace karathon {


    static PyObject* moduleSys = NULL;
    static PyObject* moduleTraceback  = NULL;
    static PyObject* functionFormatException = NULL;


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
        PyObject* obResult = NULL;
        PyObject* ptype = NULL;
        PyObject* pvalue = NULL;
        PyObject* ptraceback = NULL;
        std::string result = "";

        // Fetch parameters of error indicator
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);

        // 'import sys' module ... otherwise 'import traceback' will fail (?!)
        if (!moduleSys) {
        moduleSys = PyImport_ImportModule("sys");
            if (!moduleSys) {
                std::cerr << "*** ERROR *** getPyErrString : 'import sys' failed\n" << std::endl;
                goto finish;
            }
        }
        // 'import traceback' module ...
        if (!moduleTraceback) {
            moduleTraceback = PyImport_ImportModule("traceback");
            if (!moduleTraceback) {
                std::cerr << "*** ERROR *** getPyErrString : 'import traceback' failed\n" << std::endl;
                goto finish;
            }
        }
        // get attribute 'format_exception'
        if (!functionFormatException) {
            functionFormatException = PyObject_GetAttrString(moduleTraceback, "format_exception");
            if (!functionFormatException) {
                std::cerr << "*** ERROR *** getPyErrString : getattr(traceback,'format_exception') failed\n" << std::endl;
                goto finish;
            }
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
                result += std::string(PyUnicode_AsUTF8(item));
            }
        } else if (pvalue) {
            PyObject* pystr = PyObject_Str(pvalue);
            result = std::string(PyUnicode_AsUTF8(pystr));           
        }

    finish:

        Py_XDECREF(obResult);
        std::cerr << result << std::endl;
        return result;
    }
}

