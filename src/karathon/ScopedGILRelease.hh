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
	    if (!m_counter++)
                m_threadState = PyEval_SaveThread();
        }

        inline ~ScopedGILRelease() {
	    if (!--m_counter) {
                PyEval_RestoreThread(m_threadState);
                m_threadState = 0;
	    }
        }

    private:
        static PyThreadState* m_threadState;
	static int m_counter;
    };
}

#endif	/* SCOPEDGILRELEASE_HH */

