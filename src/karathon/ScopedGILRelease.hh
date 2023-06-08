/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   ScopedGilRelease.hh
 * Author: esenov
 *
 * Created on February 5, 2013, 2:05 PM
 */

#ifndef SCOPEDGILRELEASE_HH
#define SCOPEDGILRELEASE_HH

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
} // namespace karathon

#endif /* SCOPEDGILRELEASE_HH */
