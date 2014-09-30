/* 
 * File:   ScopedGilRelease.hh
 * Author: esenov
 *
 * Created on February 5, 2013, 2:05 PM
 */

#ifndef SCOPEDGILRELEASE_HH
#define	SCOPEDGILRELEASE_HH

#include <boost/python.hpp>

namespace karathon {

    class ScopedGILRelease {

    public:

        inline ScopedGILRelease() {
	    if (PyGILState_Check()) m_threadState = PyEval_SaveThread();
	    else m_threadState = 0;
        }

        inline ~ScopedGILRelease() {
	    if (m_threadState) PyEval_RestoreThread(m_threadState);
        }

    private:
        PyThreadState* m_threadState;
    };
}

#endif	/* SCOPEDGILRELEASE_HH */

