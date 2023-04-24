/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   PyCoreLockWrap.hh
 * Author: haufs
 *
 * Created on October 13, 2016, 6:18 PM
 */

#ifndef KARATHON_PYCORELOCKWRAP_HH
#define KARATHON_PYCORELOCKWRAP_HH

#include <karabo/core/Lock.hh>


namespace karathon {

    class LockWrap {
        boost::shared_ptr<karabo::core::Lock> m_lock;

       public:
        LockWrap(const boost::shared_ptr<karabo::core::Lock>& l) : m_lock(l){};


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
} // namespace karathon
#endif /* KARATHON_PYCORELOCKWRAP_HH */
