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
#include <karabo/xip/RawImageData.hh>
#include <karabo/xms/Slot.hh>

using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::xip;
using namespace karabo::xms;
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

    bp::class_<Slot, boost::shared_ptr<Slot> > ("Slot", bp::no_init)
            .def("getInstanceIdOfSender"
                 , (const std::string & (Slot::*)() const) &Slot::getInstanceIdOfSender
                 , bp::return_value_policy<bp::copy_const_reference>())
            ;


    bp::class_<SignalSlotable, boost::noncopyable > ("SignalSlotableIntern")
            .def(bp::init<const std::string&, const karabo::net::BrokerConnection::Pointer&>())
            ;

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

//            .def("setSenderInfo"
//                 , (void (SignalSlotable::*)(const karabo::util::Hash&))(&SignalSlotable::setSenderInfo)
//                 , (bp::arg("senderInfo")))
//            .def("getSenderInfo"
//                 , (const karabo::util::Hash & (SignalSlotable::*)() const) (&SignalSlotable::getSenderInfo)
//                 , bp::return_value_policy<bp::copy_const_reference>())
            
            .def("getSenderInfo"
                 , (const boost::shared_ptr<karabo::xms::Slot>& (SignalSlotable::*)(const std::string&)) (&SignalSlotable::getSenderInfo)
                 , bp::arg("slotFunction"), bp::return_value_policy<bp::copy_const_reference>())
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
                 , (bp::arg("instanceId"), bp::arg("instanceInfo")))

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

            .def("registerInputChannelHash"
                 , (boost::shared_ptr<Input<Hash> > (SignalSlotableWrap::*)(const std::string&, const std::string& type, const Hash&, const bp::object&, const bp::object&)) (&SignalSlotableWrap::registerInputChannel<Hash>)
                 , (bp::arg("name")
                 , bp::arg("type") = "Network"
                 , bp::arg("config") = Hash()
                 , bp::arg("onRead") = bp::object()
                 , bp::arg("onEndOfStream") = bp::object()))

            .def("registerOutputChannelHash"
                 , (boost::shared_ptr<Output<Hash> > (SignalSlotableWrap::*)(const std::string&, const std::string&, const Hash&, const bp::object&)) (&SignalSlotableWrap::registerOutputChannel<Hash>)
                 , (bp::arg("name")
                 , bp::arg("type") = "Network"
                 , bp::arg("config") = Hash()
                 , bp::arg("onOutputPossible") = bp::object()))

            .def("registerInputChannelRawImageData"
                 , (boost::shared_ptr<Input<RawImageData> > (SignalSlotableWrap::*)(const std::string&, const std::string& type, const Hash&, const bp::object&, const bp::object&)) (&SignalSlotableWrap::registerInputChannel<RawImageData>)
                 , (bp::arg("name")
                 , bp::arg("type") = "Network"
                 , bp::arg("config") = Hash()
                 , bp::arg("onRead") = bp::object()
                 , bp::arg("onEndOfStream") = bp::object()))

            .def("registerOutputChannelRawImageData"
                 , (boost::shared_ptr<Output<RawImageData> > (SignalSlotableWrap::*)(const std::string&, const std::string&, const Hash&, const bp::object&)) (&SignalSlotableWrap::registerOutputChannel<RawImageData>)
                 , (bp::arg("name")
                 , bp::arg("type") = "Network"
                 , bp::arg("config") = Hash()
                 , bp::arg("onOutputPossible") = bp::object()))

            .def("connectChannels"
                 , (bool (SignalSlotable::*)(string, const string&, string, const string&, const bool)) (&SignalSlotable::connectChannels)
                 , (bp::arg("outputInstanceId")
                 , bp::arg("outputName")
                 , bp::arg("inputInstanceId")
                 , bp::arg("inputName")
                 , bp::arg("isVerbose") = true))

            .def("disconnectChannels"
                 , (bool (SignalSlotable::*)(string, const string&, string, const string&, const bool)) (&SignalSlotable::disconnectChannels)
                 , (bp::arg("outputInstanceId")
                 , bp::arg("outputName")
                 , bp::arg("inputInstanceId")
                 , bp::arg("inputName")
                 , bp::arg("isVerbose") = true))

            .def("createInputChannelHash"
                 , (boost::shared_ptr<karabo::io::Input<Hash> > (SignalSlotableWrap::*)(const std::string&, const Hash&, const bp::object&, const bp::object&)) (&SignalSlotableWrap::createInputChannel<karabo::io::Input<Hash> >)
                 , (bp::arg("name"), bp::arg("configuration")
                 , bp::arg("onInputAvailableHandler") = bp::object()
                 , bp::arg("onEndOfStreamEventHandler") = bp::object()))

            .def("createOutputChannelHash"
                 , (boost::shared_ptr<karabo::io::Output<Hash> > (SignalSlotableWrap::*)(const std::string&, const Hash&, const bp::object&)) (&SignalSlotableWrap::createOutputChannel<karabo::io::Output<Hash> >)
                 , (bp::arg("name"), bp::arg("configuration"), bp::arg("onOutputPossibleHandler") = bp::object()))

            .def("createInputChannelRawImageData"
                 , (boost::shared_ptr<karabo::io::Input<karabo::xip::RawImageData> > (SignalSlotableWrap::*)(const std::string&, const Hash&, const bp::object&, const bp::object&)) (&SignalSlotableWrap::createInputChannel<karabo::io::Input<karabo::xip::RawImageData> >)
                 , (bp::arg("name"), bp::arg("configuration")
                 , bp::arg("onInputAvailableHandler") = bp::object()
                 , bp::arg("onEndOfStreamEventHandler") = bp::object()))

            .def("createOutputChannelRawImageData"
                 , (boost::shared_ptr<karabo::io::Output<karabo::xip::RawImageData> > (SignalSlotableWrap::*)(const std::string&, const Hash&, const bp::object&)) (&SignalSlotableWrap::createOutputChannel<karabo::io::Output<karabo::xip::RawImageData> >)
                 , (bp::arg("name"), bp::arg("configuration"), bp::arg("onOutputPossibleHandler") = bp::object()))

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

