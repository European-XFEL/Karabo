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

#ifndef KARABIND_PYCORELOCKWRAP_HH
#define KARABIND_PYCORELOCKWRAP_HH

#include <karabo/core/Lock.hh>


namespace karabind {

    class LockWrap {
        std::shared_ptr<karabo::core::Lock> m_lock;

       public:
        LockWrap(const std::shared_ptr<karabo::core::Lock>& l) : m_lock(l){};


        /**
         * Reacquire a lock if this lock was previously unlocked
         * @param recursive: allow recursive locking if true
         */
        void lock(bool recursive = false);

        /**
         * Unlock this lock
         */
        void unlock();

        /**
         * Returns if this lock is currently valid. Note that the locked
         * device will be queried through the distributed system when
         * asking for lock validity.
         */
        bool valid();
    };
} // namespace karabind
#endif /* KARABIND_PYCORELOCKWRAP_HH */
