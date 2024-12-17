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
#include "Lock.hh"

namespace karabo {
    namespace core {


        Lock::Lock(std::weak_ptr<karabo::xms::SignalSlotable> sigSlot, const std::string& deviceId, bool recursive)
            : m_sigSlot(sigSlot), m_deviceId(deviceId), m_valid(true) {
            lock_impl(recursive);
        }


        Lock::Lock(Lock&& other) : m_sigSlot(other.m_sigSlot), m_deviceId(other.m_deviceId), m_valid(other.m_valid) {
            other.m_valid = false;
        }


        Lock::~Lock() {
            unlock_impl();
        }


        void Lock::lock(bool recursive) const {
            lock_impl(recursive);
        }


        void Lock::unlock() const {
            unlock_impl();
        }


        void Lock::lock_impl(bool recursive) const {
            if (!m_valid) {
                throw KARABO_LOCK_EXCEPTION("This lock has been invalidated");
            }
            m_valid = false;


            std::shared_ptr<karabo::xms::SignalSlotable> p = m_sigSlot.lock();
            if (p) {
                const std::string& ownInstance = p->getInstanceId();
                // check that lock is empty
                {
                    try {
                        karabo::util::Hash hash;

                        p->request(m_deviceId, "slotGetConfiguration").timeout(m_lockQueryTimeout).receive(hash);

                        const std::string& lockHolder = hash.get<std::string>("lockedBy");
                        if ((!recursive && lockHolder != "") ||
                            (recursive && lockHolder != ownInstance && lockHolder != "")) {
                            throw KARABO_LOCK_EXCEPTION("Could not acquire lock on " + m_deviceId +
                                                        ", it is locked by " + lockHolder);
                        }
                    } catch (const karabo::util::ParameterException& e) {
                        KARABO_RETHROW_AS(KARABO_LOCK_EXCEPTION("Could not acquire lock on " + m_deviceId));
                    }
                }
                // try setting

                p->request(m_deviceId, "slotReconfigure", karabo::util::Hash("lockedBy", ownInstance))
                      .timeout(m_lockQueryTimeout)
                      .receive();

                // check result
                {
                    karabo::util::Hash hash;
                    p->request(m_deviceId, "slotGetConfiguration").timeout(m_lockQueryTimeout).receive(hash);
                    const std::string& lockHolder = hash.get<std::string>("lockedBy");
                    if (ownInstance != lockHolder) {
                        throw KARABO_LOCK_EXCEPTION("Could not acquire lock on " + m_deviceId + ", it is locked by " +
                                                    lockHolder);
                    }
                }
                m_valid = true;
            }
        }


        void Lock::unlock_impl() const {
            if (m_valid) {
                std::shared_ptr<karabo::xms::SignalSlotable> p = m_sigSlot.lock();
                if (p) {
                    // now we can clear the lock
                    p->call(m_deviceId, "slotClearLock");
                }
            }
        }


        bool Lock::valid() const {
            if (!m_valid) return false;

            std::shared_ptr<karabo::xms::SignalSlotable> p = m_sigSlot.lock();
            if (p) {
                const std::string& ownInstance = m_sigSlot.lock()->getInstanceId();
                karabo::util::Hash hash;

                p->request(m_deviceId, "slotGetConfiguration").timeout(m_lockQueryTimeout).receive(hash);
                const std::string& lockHolder = hash.get<std::string>("lockedBy");
                return (ownInstance == lockHolder && m_valid);
            }
            return false;
        };

    } // namespace core
} // namespace karabo
