/* 
 * File:   SignalWrap.hh
 * Author: irinak
 *
 * Created on March 19, 2012, 11:37 AM
 */

#ifndef KARATHON_SIGNALWRAP_HH
#define	KARATHON_SIGNALWRAP_HH

#include <boost/python.hpp>
#include <karabo/xms/Signal.hh>
#include <karabo/net/BrokerChannel.hh>
#include "HashWrap.hh"
#include "ScopedGILRelease.hh"

namespace bp = boost::python;

namespace karabo {

    namespace xms {
        // Forward SignalSlotable
        class SignalSlotable;
    }
}

namespace karathon {

    class SignalWrap : public karabo::xms::Signal {
    public:

        SignalWrap(const karabo::xms::SignalSlotable* signalSlotable, const karabo::net::BrokerChannel::Pointer& channel,
                   const std::string& instanceId, const std::string& signalId,
                   const int priority = KARABO_SYS_PRIO, const int messageTimeToLive = KARABO_SYS_TTL)
        : karabo::xms::Signal(signalSlotable, channel, instanceId, signalId, priority, messageTimeToLive) {
        }

        void emitPy1(const bp::object& a1) {
            karabo::util::Hash::Pointer message(new karabo::util::Hash);
            karathon::HashWrap::set(*message, "a1", a1);
            ScopedGILRelease nogil;
            send(message);
        }

        void emitPy2(const bp::object& a1, const bp::object& a2) {
            karabo::util::Hash::Pointer message(new karabo::util::Hash);
            karathon::HashWrap::set(*message, "a1", a1);
            karathon::HashWrap::set(*message, "a2", a2);
            ScopedGILRelease nogil;
            send(message);
        }

        void emitPy3(const bp::object& a1, const bp::object& a2, const bp::object& a3) {
            karabo::util::Hash::Pointer message(new karabo::util::Hash);
            karathon::HashWrap::set(*message, "a1", a1);
            karathon::HashWrap::set(*message, "a2", a2);
            karathon::HashWrap::set(*message, "a3", a3);
            ScopedGILRelease nogil;
            send(message);
        }

        void emitPy4(const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object& a4) {
            karabo::util::Hash::Pointer message(new karabo::util::Hash);
            karathon::HashWrap::set(*message, "a1", a1);
            karathon::HashWrap::set(*message, "a2", a2);
            karathon::HashWrap::set(*message, "a3", a3);
            karathon::HashWrap::set(*message, "a4", a4);
            ScopedGILRelease nogil;
            send(message);
        }
    };
}

#endif	/*  KARATHON_SIGNALWRAP_HH */

