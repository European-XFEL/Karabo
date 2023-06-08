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

} // namespace karathon


#endif /* SCOPEDGILACQUIRE_HH */
