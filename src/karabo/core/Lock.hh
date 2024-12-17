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
>>>>>>> d89f1262a64624058e79a31967c3cdde4e8c1f00
 * File:   Lock.hh
 * Author: steffen
 *
 * Created on October 2, 2016, 12:01 PM
 */

#ifndef KARABO_CORE_LOCK_HH
#define KARABO_CORE_LOCK_HH
#include "karabo/xms/SignalSlotable.hh"


namespace karabo {
    namespace core {

        class Lock {
           public:
            /**
             * Create a lock on a device. Throws a karabo::util::LockException if the lock cannot be acquired
             * @param sigSlot: a SignalSlotable instance to use for locking the remote device
             * @param deviceId: the deviceId of the device to lock
             * @param recursive: allow recursive locking if true
             */
            Lock(std::weak_ptr<karabo::xms::SignalSlotable> sigSlot, const std::string& deviceId,
                 bool recursive = false);


            /**
             * Copy construction is disabled
             * @param other
             */
            Lock(const Lock& other) = delete;
            /**
             * Move construction will invalidate the lock being moved.
             * @param other
             */
            Lock(Lock&& other);

            /**
             * The destructor unlocks the device the lock is held on if the lock is valid.
             * It is called explictly if the lock was stolen and will then throw a
             * karabo::util::LockException
             */
            virtual ~Lock();


            /**
             * Reacquire a lock if this lock was previously unlocked
             * @param recursive: allow recursive locking if true
             */
            void lock(bool recursive = false) const;

            /**
             * Unlock this lock
             */
            void unlock() const;

            /**
             * Returns if this lock is currently valid. Note that the locked
             * device will be queried through the distributed system when
             * asking for lock validity.
             */
            bool valid() const;

           private:
            /**
             * Perform locking. Calling this function leads to the following
             * remote calls:
             *
             * 1) check if we are allowed to lock: the lockedBy field on the
             * remote device is either empty, or if recursive == true contains
             * the lock requestor's device id
             *
             * 2) request locking, e.g. set the lockedBy field. This can still
             * fail if another device locked in between
             *
             * 3) check if we are the lock holder: lockedBy should now contain
             * our device id
             *
             * @param recursive: allow recursive locking if true
             */
            void lock_impl(bool recursive) const;


            /**
             * Simply calls the clearLock slot on the locked device if we are the lock-holder
             */
            void unlock_impl() const;

            std::weak_ptr<karabo::xms::SignalSlotable> m_sigSlot;
            const std::string m_deviceId;
            mutable bool m_valid;
            const int m_lockQueryTimeout = 5000;
        };


    } // namespace core
} // namespace karabo

#endif /* KARABO_CORE_LOCK_HH */
