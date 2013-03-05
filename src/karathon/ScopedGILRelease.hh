/* 
 * File:   ScopedGilRelease.hh
 * Author: esenov
 *
 * Created on February 5, 2013, 2:05 PM
 */

#ifndef SCOPEDGILRELEASE_HH
#define	SCOPEDGILRELEASE_HH

#include <boost/python.hpp>

namespace karabo {
    namespace pyexfel {

        class ScopedGILRelease {
        public:

            inline ScopedGILRelease() {
                m_threadState = PyEval_SaveThread();
            }

            inline ~ScopedGILRelease() {
                PyEval_RestoreThread(m_threadState);
                m_threadState = 0;
            }

        private:
            PyThreadState* m_threadState;
        };

    }
}

#endif	/* SCOPEDGILRELEASE_HH */

