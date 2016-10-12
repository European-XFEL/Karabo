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
#include <karabo/xms/Slot.hh>

using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::xms;
using namespace karathon;
using namespace std;
namespace bp = boost::python;


void exportPyXmsSignalSlotable() {//exposing karabo::xms::SignalSlotable 
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
                 , "\nSets number of threads that will work on the registered slots.\nRe-entry of the same slot on a different thread will never happen.\n"
                 "Only different slots may run concurrently (if nThreads > 1).\nNOTE: This function has only effect BEFORE the event loop was started.")

            .def("runEventLoop", &SignalSlotableWrap::runEventLoop, (bp::arg("heartbeatInterval") = 10, bp::arg("instanceInfo") = karabo::util::Hash()),
                 "\nUse this method if you have created a SignalSlotable instance with autostart = False and you need to provide info for event loop.\n"
                 "Example:\n\tss = SignalSlotable.create('a')\n\tinfo = Hash('type','device')\n\tinfo['classId'] = myclassId\n\tinfo['serverId'] = myserverId\n\t"
                 "info['visibility'] = ['']\n\tinfo['version'] = my_version\n\tinfo['host'] = host_name\n\tss.runEventLoop(10, info)\n"
                 )

            .def("ensureOwnInstanceIdUnique", &SignalSlotableWrap::ensureOwnInstanceIdUnique,
                 "\nIt should be called to guarantee the uniqueness on the topic\n")

            .def("stopEventLoop", &SignalSlotableWrap::stopEventLoop)

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

            .def("registerSlotCallGuardHandler", &SignalSlotableWrap::registerSlotCallGuardHandlerPy
                 , (bp::arg("handler")))

            .def("registerPerformanceStatisticsHandler", &SignalSlotableWrap::registerPerformanceStatisticsHandlerPy
                 , (bp::arg("handler")))

            .def("connect",
                 (bool (SignalSlotable::*)(const string&, const string&, const string&, const string&))(&SignalSlotable::connect),
                 (bp::arg("signalInstanceId"),
                  bp::arg("signalFunction"),
                  bp::arg("slotInstanceId"),
                  bp::arg("slotFunction")),
                 "\nUse this method to connect \"signalFunction\" issued by \"signalInstanceId\" with \"slotFunction\" belonging to \"slotInstanceId\""
                 "\n\nExample:\n\nIf we have to communicate with remote client registered as instance \"b\" and slot \"onMoin\" ...\n\n\t"
                 "ss = SignalSlotable(\"a\")\n\tss.connect(\"\", \"moin\", \"b\", \"onMoin\")\n\tss.emit(\"moin\", 12)\n"
                 )

            .def("connect",
                 (bool (SignalSlotable::*)(const string&, const string&))(&SignalSlotable::connect),
                 (bp::arg("signalFunction"),
                  bp::arg("slotFunction")))

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

            .def("registerSlot", (&SignalSlotableWrap::registerSlotPy), (bp::arg("slotFunction")))

            
            .def("registerSignal", &SignalSlotableWrap::registerSignalPy<>,
                 (bp::arg("signalFunction")))
            .def("registerSignal", &SignalSlotableWrap::registerSignalPy<bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1")))
            .def("registerSignal", &SignalSlotableWrap::registerSignalPy<bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2")))
            .def("registerSignal", &SignalSlotableWrap::registerSignalPy<bp::object, bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
            .def("registerSignal", &SignalSlotableWrap::registerSignalPy<bp::object, bp::object, bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

            .def("registerSystemSignal", &SignalSlotableWrap::registerSystemSignalPy<>,
                 (bp::arg("signalFunction")))
            .def("registerSystemSignal", &SignalSlotableWrap::registerSystemSignalPy<bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1")))
            .def("registerSystemSignal", &SignalSlotableWrap::registerSystemSignalPy<bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2")))
            .def("registerSystemSignal", &SignalSlotableWrap::registerSystemSignalPy<bp::object, bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
            .def("registerSystemSignal", &SignalSlotableWrap::registerSystemSignalPy<bp::object, bp::object, bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

            .def("emit", &SignalSlotableWrap::emitPy<>,
                 (bp::arg("signalFunction")))
            .def("emit", &SignalSlotableWrap::emitPy<bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1")))
            .def("emit", &SignalSlotableWrap::emitPy<bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2")))
            .def("emit", &SignalSlotableWrap::emitPy<bp::object, bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
            .def("emit", &SignalSlotableWrap::emitPy<bp::object, bp::object, bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

            .def("call", &SignalSlotableWrap::callPy<>,
                 (bp::arg("signalFunction")))
            .def("call", &SignalSlotableWrap::callPy<bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1")))
            .def("call", &SignalSlotableWrap::callPy<bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2")))
            .def("call", &SignalSlotableWrap::callPy<bp::object, bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
            .def("call", &SignalSlotableWrap::callPy<bp::object, bp::object, bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

            .def("request", &SignalSlotableWrap::requestPy<>,
                 (bp::arg("signalFunction")))
            .def("request", &SignalSlotableWrap::requestPy<bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1")))
            .def("request", &SignalSlotableWrap::requestPy<bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2")))
            .def("request", &SignalSlotableWrap::requestPy<bp::object, bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
            .def("request", &SignalSlotableWrap::requestPy<bp::object, bp::object, bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

            .def("requestNoWait", &SignalSlotableWrap::requestNoWaitPy<>,
                 (bp::arg("signalFunction")))
            .def("requestNoWait", &SignalSlotableWrap::requestNoWaitPy<bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1")))
            .def("requestNoWait", &SignalSlotableWrap::requestNoWaitPy<bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2")))
            .def("requestNoWait", &SignalSlotableWrap::requestNoWaitPy<bp::object, bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
            .def("requestNoWait", &SignalSlotableWrap::requestNoWaitPy<bp::object, bp::object, bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

            .def("reply", &SignalSlotableWrap::replyPy<>,
                 (bp::arg("signalFunction")))
            .def("reply", &SignalSlotableWrap::replyPy<bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1")))
            .def("reply", &SignalSlotableWrap::replyPy<bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2")))
            .def("reply", &SignalSlotableWrap::replyPy<bp::object, bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
            .def("reply", &SignalSlotableWrap::replyPy<bp::object, bp::object, bp::object, bp::object>,
                 (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

            .def("createOutputChannel", &SignalSlotableWrap::createOutputChannelPy,
                 (bp::arg("channelName"), bp::arg("configuration"), bp::arg("handler") = bp::object()))

            .def("createInputChannel", &SignalSlotableWrap::createInputChannelPy,
                 (bp::arg("channelName"), bp::arg("configuration"),
                  bp::arg("onData") = bp::object(), bp::arg("onInput") = bp::object(),
                  bp::arg("onEndOfStream") = bp::object()))

            .def("connectInputChannels", &SignalSlotableWrap::connectInputChannelsPy)

            .def("getOutputChannel", &SignalSlotableWrap::getOutputChannelPy, (bp::arg("channelName")))

            .def("getInputChannel", &SignalSlotableWrap::getInputChannelPy, (bp::arg("channelName")))

            .def("registerDataHandler", &SignalSlotableWrap::registerDataHandlerPy,
                 (bp::arg("channelName"), bp::arg("handlerPerData") = bp::object()))

            .def("registerInputHandler", &SignalSlotableWrap::registerInputHandlerPy,
                 (bp::arg("channelName"), bp::arg("handlerPerInput") = bp::object()))

            .def("registerEndOfStreamHandler", &SignalSlotableWrap::registerEndOfStreamHandlerPy,
                 (bp::arg("channelName"), bp::arg("handler") = bp::object()))

            .def("exists",
                 (bp::tuple(SignalSlotableWrap::*)(const std::string&))(&SignalSlotableWrap::existsPy),
                 (bp::arg("instanceId")))

            .def("getAccessLevel",
                 (int (SignalSlotable::*)(const std::string&) const) (&SignalSlotable::getAccessLevel),
                 (bp::arg("instanceId")))

            .def("getBrokerHost", &SignalSlotableWrap::getBrokerHost)
            .def("getBrokerHosts", &SignalSlotableWrap::getBrokerHosts)
            .def("getBrokerTopic", &SignalSlotableWrap::getBrokerTopic)
            ;
}

