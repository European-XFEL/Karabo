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
using namespace karathon;
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
            .def(bp::init<const std::string&, const karabo::net::BrokerConnection::Pointer&>())
            ;

    typedef karabo::io::Input<karabo::util::Hash> InputHashType;
    typedef karabo::io::Output<karabo::util::Hash> OutputHashType;
    
    bp::class_<SignalSlotableWrap, boost::shared_ptr<SignalSlotableWrap>, bp::bases< SignalSlotable>, boost::noncopyable > ("SignalSlotable")
            .def(bp::init<>())
            .def(bp::init<const std::string&>())
            .def(bp::init<const std::string&, const std::string&>())
            .def(bp::init<const std::string&, const std::string&, const karabo::util::Hash&>())
            .def(bp::init<const std::string&, const std::string&, const karabo::util::Hash&, const bool >())
            .def(bp::init<const std::string&, const std::string&, const karabo::util::Hash&, const bool, const int >())

            .def("create", &SignalSlotableWrap::create,
                 (bp::arg("instanceId"),
                 bp::arg("connectionType") = "Jms",
                 bp::arg("connectionParameters") = karabo::util::Hash(),
                 bp::arg("autostart") = false,
                 bp::arg("heartbeatInterval") = 10
                 ),
                 "\nUse this factory method to create SignalSlotable object with given 'instanceId', 'connectionType', 'connectionParameters' and 'autostart' of event loop (by default, no start).\n"
                 "Example:\n\tss = SignalSlotable.create('a')\n"
                 ).staticmethod("create")

            .def("login"
                 , (bool (SignalSlotable::*)(const std::string&, const std::string&, const std::string&)) (&SignalSlotable::login)
                 , (bp::arg("username"), bp::arg("password"), bp::arg("provider")))
            .def("logout", (bool (SignalSlotable::*)())(&SignalSlotable::logout))

            .def("runEventLoop", &SignalSlotableWrap::runEventLoop, (bp::arg("heartbeatInterval") = 10, bp::arg("instanceInfo") = karabo::util::Hash()),
                 "\nUse this method if you have created a SignalSlotable instance with autostart = False and you need to provide info for event loop.\n"
                 "Example:\n\tss = SignalSlotable.create('a')\n\tinfo = Hash('type','device')\n\tinfo['classId'] = myclassId\n\tinfo['serverId'] = myserverId\n\t"
                 "info['visibility'] = ['']\n\tinfo['version'] = my_version\n\tinfo['host'] = host_name\n\tss.runEventLoop(10, info)\n"
                 )

            .def("stopEventLoop", &SignalSlotableWrap::stopEventLoop)

            .def("setSenderInfo"
                 , (void (SignalSlotable::*)(const karabo::util::Hash&))(&SignalSlotable::setSenderInfo)
                 , (bp::arg("senderInfo")))
            .def("getSenderInfo"
                 , (const karabo::util::Hash & (SignalSlotable::*)() const) (&SignalSlotable::getSenderInfo)
                 , bp::return_value_policy<bp::copy_const_reference>())
            .def("getInstanceId"
                 , (bp::object(SignalSlotableWrap::*)()) & SignalSlotableWrap::getInstanceId)
            .def("updateInstanceInfo"
                 , (void (SignalSlotable::*)(const karabo::util::Hash&))(&SignalSlotable::updateInstanceInfo)
                 , (bp::arg("update")))
            .def("getInstanceInfo"
                 , (const karabo::util::Hash & (SignalSlotable::*)() const) (&SignalSlotable::getInstanceInfo)
                 , bp::return_value_policy<bp::copy_const_reference>())
            .def("trackExistenceOfInstance"
                 , (void (SignalSlotable::*)(const std::string&))(&SignalSlotable::trackExistenceOfInstance)
                 , (bp::arg("instanceId")))
            .def("stopTrackingExistenceOfInstance"
                 , (void (SignalSlotable::*)(const std::string&))(&SignalSlotable::stopTrackingExistenceOfInstance)
                 , (bp::arg("instanceId")))

            .def("connect",
                 (bool (SignalSlotable::*)(const string, const string&, const string, const string&, SignalSlotable::ConnectionType, const bool))(&SignalSlotable::connect),
                 (bp::arg("signalInstanceId"),
                 bp::arg("signalFunction"),
                 bp::arg("slotInstanceId"),
                 bp::arg("slotFunction"),
                 bp::arg("connectionType") = SignalSlotable::TRACK,
                 bp::arg("isVerbose") = true),
                 "\nUse this method to connect \"signalFunction\" issued by \"signalInstanceId\" with \"slotFunction\" belong to \"slotInstanceId\" using \"connectionType\"\n"
                 "and \"isVerbose\" flag controlling verbosity level.\n\nExample:\n\nIf we have to communicate with remote client registered as instance \"b\" and slot \"onMoin\" ...\n\n\t"
                 "ss = SignalSlotable(\"a\")\n\tss.connect(\"\", \"moin\", \"b\", \"onMoin\")\n\tss.emit(\"moin\", 12)\n"
                 )

            .def("connect",
                 (bool (SignalSlotable::*)(const string&, const string&, SignalSlotable::ConnectionType, const bool))(&SignalSlotable::connect),
                 (bp::arg("signalFunction"),
                 bp::arg("slotFunction"),
                 bp::arg("connectionType") = SignalSlotable::TRACK,
                 bp::arg("isVerbose") = true))

            .def("updateInstanceInfo",
                 (void (SignalSlotable::*)(const Hash&))(&SignalSlotable::updateInstanceInfo),
                 (bp::arg("updateHash")))

            .def("getAvailableInstances", &SignalSlotableWrap::getAvailableInstancesPy, (bp::arg("activateTracking") = false))
            .def("getAvailableSignals", &SignalSlotableWrap::getAvailableSignalsPy, bp::arg("instanceId"))
            .def("getAvailableSlots", &SignalSlotableWrap::getAvailableSlotsPy, bp::arg("instanceId"))

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

            .def("createInputChannelHash"
                 , (boost::shared_ptr<InputHashType> (SignalSlotableWrap::*)(const std::string&, const Hash&, const bp::object&, const bp::object&, const boost::shared_ptr<InputHashType>&)) (&SignalSlotableWrap::createInputChannel<InputHashType>)
                 , (bp::arg("name"), bp::arg("configuration")
                 , bp::arg("onInputAvailableHandler") = bp::object()
                 , bp::arg("onEndOfStreamEventHandler") = bp::object()
                 , bp::arg("never_use") = boost::shared_ptr<InputHashType>()))
            
            .def("createOutputChannelHash"
                 , (boost::shared_ptr<OutputHashType> (SignalSlotableWrap::*)(const std::string&, const Hash&, const bp::object&, const boost::shared_ptr<OutputHashType>&))(&SignalSlotableWrap::createOutputChannel<OutputHashType>)
                 , (bp::arg("name"), bp::arg("configuration"), bp::arg("onOutputPossibleHandler") = bp::object(), bp::arg("never_use") = boost::shared_ptr<OutputHashType>()))
            
            .def("getInputChannels", &SignalSlotableWrap::getInputChannels)
            
            .def("getOutputChannels", &SignalSlotableWrap::getOutputChannels)
            
            .def("exists"
                 , (bp::tuple(SignalSlotableWrap::*)(const std::string&))(&SignalSlotableWrap::exists)
                 , (bp::arg("instanceId")))
            
            .def("getAccessLevel"
                 , (int (SignalSlotable::*)(const std::string&) const) (&SignalSlotable::getAccessLevel)
                 , (bp::arg("instanceId")))
            
            .def("connectInputChannels"
                , (void (SignalSlotable::*)()) (&SignalSlotable::connectInputChannels))
            ;
}

