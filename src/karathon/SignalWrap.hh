/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   SignalWrap.hh
 * Author: irinak
 *
 * Created on March 19, 2012, 11:37 AM
 */

#ifndef KARATHON_SIGNALWRAP_HH
#define KARATHON_SIGNALWRAP_HH

#include <boost/python.hpp>
#include <karabo/net/Broker.hh>
#include <karabo/xms/Signal.hh>

#include "HashWrap.hh"
#include "ScopedGILRelease.hh"

namespace bp = boost::python;

namespace karabo {

    namespace xms {
        // Forward SignalSlotable
        class SignalSlotable;
    } // namespace xms
} // namespace karabo

namespace karathon {

    class SignalWrap : public karabo::xms::Signal {
       public:
        SignalWrap(const karabo::xms::SignalSlotable* signalSlotable, const karabo::net::Broker::Pointer& producer,
                   const std::string& instanceId, const std::string& signalId, const int priority = KARABO_SYS_PRIO,
                   const int messageTimeToLive = KARABO_SYS_TTL)
            : karabo::xms::Signal(signalSlotable, producer, instanceId, signalId, priority, messageTimeToLive) {}

        template <typename... Args>
        void emitPy(const Args&... args) {
            auto body = boost::make_shared<karabo::util::Hash>();
            packPy(*body, args...);
            ScopedGILRelease nogil;
            emit(body);
        }
    };
} // namespace karathon

#endif /*  KARATHON_SIGNALWRAP_HH */
