/*
 * File:   ScopedGILAcquire.hh
 * Author: esenov
 *
 * Created on February 25, 2013, 2:54 PM
 */

#ifndef SCOPEDGILACQUIRE_HH
#define SCOPEDGILACQUIRE_HH

#include <boost/python.hpp>

namespace karathon {

    class ScopedGILAcquire {
       public:
        inline ScopedGILAcquire() : m_gstate(PyGILState_Ensure()) {}

        inline ~ScopedGILAcquire() {
            PyGILState_Release(m_gstate);
        }

       private:
        PyGILState_STATE m_gstate;
    };

    /**
     * Extract traceback using "error indicator"
     * NOTE: It should be called under the GIL protection!
     * @return python traceback represented as a (multi-lined) string
     */
    std::string getPythonExceptionAsString();

} // namespace karathon


#endif /* SCOPEDGILACQUIRE_HH */
