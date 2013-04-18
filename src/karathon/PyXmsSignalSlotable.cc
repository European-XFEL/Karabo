/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>,
 * contributions by <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <karabo/xms/SignalSlotable.hh>
#include "SignalSlotableWrap.hh"

using namespace karabo::xms;
using namespace karabo::util;
using namespace karabo::pyexfel;
using namespace std;
namespace bp = boost::python;


void exportPyXmsSignalSlotable() {//exposing karabo::xms::SignalSlotable 
    bp::enum_< SignalSlotable::SlotType > ("SlotType")
            .value("SPECIFIC", SignalSlotable::SPECIFIC)
            .value("GLOBAL", SignalSlotable::GLOBAL)
            .export_values()
            ;

    bp::enum_< SignalSlotable::ConnectionType > ("ConnectionType")
            .value("NO_TRACK", SignalSlotable::NO_TRACK)
            .value("TRACK", SignalSlotable::TRACK)
            .value("RECONNECT", SignalSlotable::RECONNECT)
            .export_values()
            ;

    bp::class_<SignalSlotable, boost::noncopyable > ("SignalSlotableIntern")
            .def(bp::init<const karabo::net::BrokerConnection::Pointer&, const std::string&, const karabo::util::Hash&, int>())
            .def(bp::init<const karabo::net::BrokerConnection::Pointer&, const std::string&, const karabo::util::Hash&>())
            .def(bp::init<const karabo::net::BrokerConnection::Pointer&, const std::string&>())
            ;

    bp::class_<SignalSlotableWrap, boost::shared_ptr<SignalSlotableWrap>, bp::bases< SignalSlotable>, boost::noncopyable > ("SignalSlotable")

            .def(bp::init<const std::string&>())
            .def(bp::init<const std::string&, const std::string&>())
            .def(bp::init<const std::string&, const std::string&, const karabo::util::Hash&>())

            .def("create", &SignalSlotableWrap::create,
                 (bp::arg("instanceId") = "py/console/0",
                 bp::arg("connectionType") = "Jms",
                 bp::arg("connectionParameters") = karabo::util::Hash()
                 )).staticmethod("create")

            .def("connect",
                 (bool (SignalSlotable::*)(const string, const string&, const string, const string&, SignalSlotable::ConnectionType, const bool))(&SignalSlotable::connect),
                 (bp::arg("signalInstanceId"),
                 bp::arg("signalFunction"),
                 bp::arg("slotInstanceId"),
                 bp::arg("slotFunction"),
                 bp::arg("connectionType") = SignalSlotable::TRACK,
                 bp::arg("isVerbose") = true))

            .def("connect",
                 (bool (SignalSlotable::*)(const string&, const string&, SignalSlotable::ConnectionType, const bool))(&SignalSlotable::connect),
                 (bp::arg("signalFunction"),
                 bp::arg("slotFunction"),
                 bp::arg("connectionType") = SignalSlotable::TRACK,
                 bp::arg("isVerbose") = true))

            .def("getAvailableInstances", &SignalSlotableWrap::getAvailableInstancesPy)
            .def("getAvailableSignals", &SignalSlotableWrap::getAvailableSignalsPy, bp::arg("instanceId"))
            .def("getAvailableSlots", &SignalSlotableWrap::getAvailableSlotsPy, bp::arg("instanceId"))

            .def("slotPing", (void (SignalSlotable::*)()) (&SignalSlotable::slotPing))
            .def("disconnect", (void (SignalSlotable::*)(string const &, string const &)) (&SignalSlotable::disconnect), (bp::arg("signal"), bp::arg("slot")))
            .def("getInstanceId"
                 , (string const & (SignalSlotable::*)() const) (&SignalSlotable::getInstanceId)
                 , bp::return_value_policy< bp::copy_const_reference > ())

            .def("registerSlot", (&SignalSlotableWrap::registerSlotPy), (bp::arg("slotFunction"), bp::arg("slotType") = SignalSlotable::SPECIFIC))

            //.def("registerSlot", (&SignalSlotableWrap::registerMemberSlotPy), (bp::arg("slotFunction"), bp::arg("selfObject"), bp::arg("slotType") = SignalSlotable::SPECIFIC))

            .def("registerSignal", &SignalSlotableWrap::registerSignalPy0, (bp::arg("signalFunction")))
            .def("registerSignal", &SignalSlotableWrap::registerSignalPy1, (bp::arg("signalFunction"), bp::arg("a1")))
            .def("registerSignal", &SignalSlotableWrap::registerSignalPy2, (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2")))
            .def("registerSignal", &SignalSlotableWrap::registerSignalPy3, (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
            .def("registerSignal", &SignalSlotableWrap::registerSignalPy4, (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

            .def("call", (void (SignalSlotable::*)(string, string const &) const) (&SignalSlotable::call),
                 (bp::arg("instanceId"), bp::arg("functionName")))

            .def("call", (void (SignalSlotableWrap::*)(string, string const &, const bp::object&) const) (&SignalSlotableWrap::callPy1),
                 (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1")))

            .def("call", (void (SignalSlotableWrap::*)(string, string const &, const bp::object&, const bp::object&) const) (&SignalSlotableWrap::callPy2),
                 (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2")))

            .def("call", (void (SignalSlotableWrap::*)(string, string const &, const bp::object&, const bp::object&, const bp::object&) const) (&SignalSlotableWrap::callPy3),
                 (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))

            .def("call", (void (SignalSlotableWrap::*)(string, string const &, const bp::object&, const bp::object&, const bp::object&, const bp::object&) const) (&SignalSlotableWrap::callPy4),
                 (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

            .def("request", (RequestorWrap(SignalSlotableWrap::*)(string, const string&)) (&SignalSlotableWrap::requestPy0),
                 (bp::arg("instanceId"), bp::arg("functionName")))

            .def("request", (RequestorWrap(SignalSlotableWrap::*)(string, const string&, const bp::object&)) (&SignalSlotableWrap::requestPy1),
                 (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1")))

            .def("request", (RequestorWrap(SignalSlotableWrap::*)(string, const string&, const bp::object&, const bp::object&)) (&SignalSlotableWrap::requestPy2),
                 (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2")))

            .def("request", (RequestorWrap(SignalSlotableWrap::*)(string, const string&, const bp::object&, const bp::object&, const bp::object&)) (&SignalSlotableWrap::requestPy3),
                 (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))

            .def("request", (RequestorWrap(SignalSlotableWrap::*)(string, const string&, const bp::object&, const bp::object&, const bp::object&, const bp::object&)) (&SignalSlotableWrap::requestPy4),
                 (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

            .def("reply", (void (SignalSlotableWrap::*)()) (&SignalSlotableWrap::replyPy0))

            .def("reply", (void (SignalSlotableWrap::*)(const bp::object&)) (&SignalSlotableWrap::replyPy1),
                 (bp::arg("a1")))

            .def("reply", (void (SignalSlotableWrap::*)(const bp::object&, const bp::object&)) (&SignalSlotableWrap::replyPy2),
                 (bp::arg("a1"), bp::arg("a2")))

            .def("reply", (void (SignalSlotableWrap::*)(const bp::object&, const bp::object&, const bp::object&)) (&SignalSlotableWrap::replyPy3),
                 (bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))

            .def("reply", (void (SignalSlotableWrap::*)(const bp::object&, const bp::object&, const bp::object&, const bp::object&)) (&SignalSlotableWrap::replyPy4),
                 (bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))



            .def("emit", (void (SignalSlotable::*)(string const &) const) (&SignalSlotable::emit), (bp::arg("signalFunction")))
            .def("emit", &SignalSlotableWrap::emitPy1, (bp::arg("signalFunction"), bp::arg("a1")))
            .def("emit", &SignalSlotableWrap::emitPy2, (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2")))
            .def("emit", &SignalSlotableWrap::emitPy3, (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
            .def("emit", &SignalSlotableWrap::emitPy4, (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))
            ;
}

