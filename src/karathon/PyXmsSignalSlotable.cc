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
            .value("LOCAL", SignalSlotable::LOCAL)
            .value("GLOBAL", SignalSlotable::GLOBAL)
            .export_values()
            ;

    bp::enum_< SignalSlotable::ConnectionType > ("ConnectionType")
            .value("NO_TRACK", SignalSlotable::NO_TRACK)
            .value("TRACK", SignalSlotable::TRACK)
            .value("RECONNECT", SignalSlotable::RECONNECT)
            .export_values()
            ;

    bp::class_<Slot, boost::shared_ptr<Slot>, boost::noncopyable> ("Slot", bp::no_init)
            .def("getInstanceIdOfSender"
                 , (const std::string & (Slot::*)() const) &Slot::getInstanceIdOfSender
                 , bp::return_value_policy<bp::copy_const_reference>())
            ;

    bp::class_<SignalSlotableWrap::RequestorWrap > ("Requestor", bp::no_init)
            .def("waitForReply", (&SignalSlotableWrap::RequestorWrap::waitForReply), (bp::arg("milliseconds")))

            .def("receiveAsync0"
                 , (void(SignalSlotableWrap::RequestorWrap::*)(const bp::object&))(&SignalSlotableWrap::RequestorWrap::receiveAsyncPy0)
                 , (bp::arg("replyCallback")))
            .def("receiveAsync1"
                 , (void(SignalSlotableWrap::RequestorWrap::*)(const bp::object&))(&SignalSlotableWrap::RequestorWrap::receiveAsyncPy1)
                 , (bp::arg("replyCallback")))
            .def("receiveAsync2"
                 , (void(SignalSlotableWrap::RequestorWrap::*)(const bp::object&))(&SignalSlotableWrap::RequestorWrap::receiveAsyncPy2)
                 , (bp::arg("replyCallback")))
            .def("receiveAsync3"
                 , (void(SignalSlotableWrap::RequestorWrap::*)(const bp::object&))(&SignalSlotableWrap::RequestorWrap::receiveAsyncPy3)
                 , (bp::arg("replyCallback")))
            .def("receiveAsync4"
                 , (void(SignalSlotableWrap::RequestorWrap::*)(const bp::object&))(&SignalSlotableWrap::RequestorWrap::receiveAsyncPy4)
                 , (bp::arg("replyCallback")))

            // The following functions are "similar" C++ API. They work but considered not useful: use waitForReply 
            //.def("timeout", (&SignalSlotableWrap::RequestorWrap::timeoutPy), (bp::arg("milliseconds")))
            //.def("receive0", (bp::tuple(SignalSlotableWrap::RequestorWrap::*)())(&SignalSlotableWrap::RequestorWrap::receivePy0))
            //.def("receive1", (bp::tuple(SignalSlotableWrap::RequestorWrap::*)())(&SignalSlotableWrap::RequestorWrap::receivePy1))
            //.def("receive2", (bp::tuple(SignalSlotableWrap::RequestorWrap::*)())(&SignalSlotableWrap::RequestorWrap::receivePy2))
            //.def("receive3", (bp::tuple(SignalSlotableWrap::RequestorWrap::*)())(&SignalSlotableWrap::RequestorWrap::receivePy3))
            //.def("receive4", (bp::tuple(SignalSlotableWrap::RequestorWrap::*)())(&SignalSlotableWrap::RequestorWrap::receivePy4))

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
            
            .def("setNumberOfThreads"
                 , (void (SignalSlotable::*)(int))(&SignalSlotable::setNumberOfThreads)
                 , (bp::arg("nthreads"))
                 ,"\nSets number of threads that will work on the registered slots.\nRe-entry of the same slot on a different thread will never happen.\n"
                  "Only different slots may run concurrently (if nThreads > 1).\nNOTE: This function has only effect BEFORE the event loop was started.")

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

            .def("registerInstanceNotAvailableHandler", &SignalSlotableWrap::registerInstanceNotAvailableHandlerPy
                 , (bp::arg("handler")))
            
            .def("registerInstanceAvailableAgainHandler", &SignalSlotableWrap::registerInstanceAvailableAgainHandlerPy
                 , (bp::arg("handler")))
            
            .def("registerExceptionHandler", &SignalSlotableWrap::registerExceptionHandlerPy
                 , (bp::arg("handler")))
            
            //.def("registerInstanceNewHandler", &SignalSlotableWrap::registerInstanceNewHandlerPy
            //     , (bp::arg("handler")))
            
            .def("registerSlotCallGuardHandler", &SignalSlotableWrap::registerSlotCallGuardHandlerPy
                 , (bp::arg("handler")))
             
            .def("registerPerformanceStatisticsHandler", &SignalSlotableWrap::registerPerformanceStatisticsHandlerPy
                 , (bp::arg("handler")))
            
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

            .def("registerSlot", (&SignalSlotableWrap::registerSlotPy), (bp::arg("slotFunction"), bp::arg("slotType") = SignalSlotable::LOCAL))

            //.def("registerSlot", (&SignalSlotableWrap::registerMemberSlotPy), (bp::arg("slotFunction"), bp::arg("selfObject"), bp::arg("slotType") = SignalSlotable::SPECIFIC))

            .def("registerSignal", &SignalSlotableWrap::registerSignalPy0, (bp::arg("signalFunction")))
            .def("registerSignal", &SignalSlotableWrap::registerSignalPy1, (bp::arg("signalFunction"), bp::arg("a1")))
            .def("registerSignal", &SignalSlotableWrap::registerSignalPy2, (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2")))
            .def("registerSignal", &SignalSlotableWrap::registerSignalPy3, (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
            .def("registerSignal", &SignalSlotableWrap::registerSignalPy4, (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

            .def("emit", (void (SignalSlotable::*)(string const &) const) (&SignalSlotable::emit)
                 , (bp::arg("signalFunction")))
            .def("emit", &SignalSlotableWrap::emitPy1
                 , (bp::arg("signalFunction"), bp::arg("a1")))
            .def("emit", &SignalSlotableWrap::emitPy2
                 , (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2")))
            .def("emit", &SignalSlotableWrap::emitPy3
                 , (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
            .def("emit", &SignalSlotableWrap::emitPy4
                 , (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

            .def("call", (void (SignalSlotable::*)(string, string const &) const) (&SignalSlotable::call)
                 , (bp::arg("instanceId"), bp::arg("functionName")))

            .def("call", (void (SignalSlotableWrap::*)(string, string const &, const bp::object&) const)
                 (&SignalSlotableWrap::callPy1)
                 , (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1")))

            .def("call", (void (SignalSlotableWrap::*)(string, string const &, const bp::object&, const bp::object&) const)
                 (&SignalSlotableWrap::callPy2)
                 , (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2")))

            .def("call", (void (SignalSlotableWrap::*)(string, string const &, const bp::object&, const bp::object&, const bp::object&) const)
                 (&SignalSlotableWrap::callPy3)
                 , (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))

            .def("call"
                 , (void (SignalSlotableWrap::*)(string, string const &, const bp::object&, const bp::object&, const bp::object&, const bp::object&) const)
                 (&SignalSlotableWrap::callPy4)
                 , (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

            .def("request"
                 , (SignalSlotableWrap::RequestorWrap(SignalSlotableWrap::*)(string, const string&))
                 (&SignalSlotableWrap::requestPy0)
                 , (bp::arg("instanceId"), bp::arg("functionName")))

            .def("request"
                 , (SignalSlotableWrap::RequestorWrap(SignalSlotableWrap::*)(string, const string&, const bp::object&))
                 (&SignalSlotableWrap::requestPy1)
                 , (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1")))

            .def("request", (SignalSlotableWrap::RequestorWrap(SignalSlotableWrap::*)(string, const string&, const bp::object&, const bp::object&)) (&SignalSlotableWrap::requestPy2),
                 (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2")))

            .def("request", (SignalSlotableWrap::RequestorWrap(SignalSlotableWrap::*)(string, const string&, const bp::object&, const bp::object&, const bp::object&)) (&SignalSlotableWrap::requestPy3),
                 (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))

            .def("request", (SignalSlotableWrap::RequestorWrap(SignalSlotableWrap::*)(string, const string&, const bp::object&, const bp::object&, const bp::object&, const bp::object&)) (&SignalSlotableWrap::requestPy4),
                 (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

            .def("requestNoWait"
                 , (SignalSlotableWrap::RequestorWrap(SignalSlotableWrap::*)(string, const string&, string, const string&))(&SignalSlotableWrap::requestNoWaitPy0)
                 , (bp::arg("requestSlotInstanceId"), bp::arg("requestSlotFunction"), bp::arg("replySlotInstanceId"), bp::arg("replySlotFunction")))

            .def("requestNoWait"
                 , (SignalSlotableWrap::RequestorWrap(SignalSlotableWrap::*)(string, const string&, string, const string&, const bp::object&))
                 (&SignalSlotableWrap::requestNoWaitPy1)
                 , (bp::arg("requestSlotInstanceId"), bp::arg("requestSlotFunction")
                 , bp::arg("replySlotInstanceId"), bp::arg("replySlotFunction")
                 , bp::arg("a1")))

            .def("requestNoWait"
                 , (SignalSlotableWrap::RequestorWrap(SignalSlotableWrap::*)(string, const string&, string, const string&, const bp::object&, const bp::object&))
                 (&SignalSlotableWrap::requestNoWaitPy2)
                 , (bp::arg("requestSlotInstanceId"), bp::arg("requestSlotFunction"), bp::arg("replySlotInstanceId"), bp::arg("replySlotFunction")
                 , bp::arg("a1"), bp::arg("a2")))

            .def("requestNoWait"
                 , (SignalSlotableWrap::RequestorWrap(SignalSlotableWrap::*)
                 (string, const string&, string, const string&, const bp::object&, const bp::object&, const bp::object&))
                 (&SignalSlotableWrap::requestNoWaitPy3)
                 , (bp::arg("requestSlotInstanceId"), bp::arg("requestSlotFunction"), bp::arg("replySlotInstanceId"), bp::arg("replySlotFunction")
                 , bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))

            .def("requestNoWait"
                 , (SignalSlotableWrap::RequestorWrap(SignalSlotableWrap::*)
                 (string, const string&, string, const string&, const bp::object&, const bp::object&, const bp::object&, const bp::object&))
                 (&SignalSlotableWrap::requestNoWaitPy4)
                 , (bp::arg("requestSlotInstanceId"), bp::arg("requestSlotFunction"), bp::arg("replySlotInstanceId"), bp::arg("replySlotFunction")
                 , bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

            .def("reply", (void (SignalSlotableWrap::*)()) (&SignalSlotableWrap::replyPy0))

            .def("reply", (void (SignalSlotableWrap::*)(const bp::object&)) (&SignalSlotableWrap::replyPy1)
                 , (bp::arg("a1")))

            .def("reply", (void (SignalSlotableWrap::*)(const bp::object&, const bp::object&)) (&SignalSlotableWrap::replyPy2)
                 , (bp::arg("a1"), bp::arg("a2")))

            .def("reply", (void (SignalSlotableWrap::*)(const bp::object&, const bp::object&, const bp::object&)) (&SignalSlotableWrap::replyPy3)
                 , (bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))

            .def("reply", (void (SignalSlotableWrap::*)(const bp::object&, const bp::object&, const bp::object&, const bp::object&)) (&SignalSlotableWrap::replyPy4)
                 , (bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))


            .def("createOutputChannel", &SignalSlotableWrap::createOutputChannelPy
                 , (bp::arg("channelName"), bp::arg("configuration"), bp::arg("handler") = bp::object()))

            .def("createInputChannel", &SignalSlotableWrap::createInputChannelPy
                 , (bp::arg("channelName"), bp::arg("configuration"), bp::arg("onData") = bp::object(), bp::arg("onEndOfStream") = bp::object()))

            .def("connectInputChannels", &SignalSlotable::connectInputChannels)
            
            .def("exists"
                 , (bp::tuple(SignalSlotableWrap::*)(const std::string&))(&SignalSlotableWrap::existsPy)
                 , (bp::arg("instanceId")))

            .def("getAccessLevel"
                 , (int (SignalSlotable::*)(const std::string&) const) (&SignalSlotable::getAccessLevel)
                 , (bp::arg("instanceId")))

            .def("getBrokerHost", &SignalSlotableWrap::getBrokerHost)
            .def("getBrokerHosts", &SignalSlotableWrap::getBrokerHosts)
            .def("getBrokerTopic", &SignalSlotableWrap::getBrokerTopic)

            ;
}

