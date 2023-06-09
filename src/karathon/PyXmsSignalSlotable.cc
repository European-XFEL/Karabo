/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>,
 * contributions by <irina.kozlova@xfel.eu>
 *
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

#include <boost/python.hpp>
#include <karabo/xms/SignalSlotable.hh>
#include <karabo/xms/Slot.hh>

#include "SignalSlotableWrap.hh"

using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::xms;
using namespace karathon;
using namespace std;
namespace bp = boost::python;


void exportPyXmsSignalSlotable() { // exposing karabo::xms::SignalSlotable
    bp::class_<Slot, boost::shared_ptr<Slot>, boost::noncopyable>("Slot", bp::no_init)
          .def("getInstanceIdOfSender", (const std::string& (Slot::*)() const) & Slot::getInstanceIdOfSender,
               bp::return_value_policy<bp::copy_const_reference>());

    bp::class_<SignalSlotableWrap::RequestorWrap>("Requestor", bp::no_init)
          .def("waitForReply", (&SignalSlotableWrap::RequestorWrap::waitForReply), (bp::arg("milliseconds")))

          .def("receiveAsync", &SignalSlotableWrap::RequestorWrap::receiveAsyncPy,
               (bp::arg("replyCallback"), bp::arg("errorCallback") = bp::object(), bp::arg("timeoutMs") = bp::object(),
                bp::arg("numCallbackArgs") = bp::object()),
               "Asynchronously receive the reply to the requested slot via a callback\n\n"
               "replyCallback   - called when slot reply arrives, argument number must match number replied by slot\n"
               "errorCallback   - if specified, called when the slot replies an error or the reply times out,\n"
               "                  will receive two str arguments:\n"
               "                  short description of the problem and details (e.g. stack trace)\n"
               "timeoutMs       - specify timeout in milliseconds (otherwise a long default is used)\n"
               "numCallbackArgs - only needed to specify if number of arguments cannot be deduced from replyCallback")
          // Keep the following (with number of return arguments in function name, but without error handling) for
          // backward compatibility
          .def("receiveAsync0", &SignalSlotableWrap::RequestorWrap::receiveAsyncPy0, (bp::arg("replyCallback")),
               "Use receiveAsync instead")
          .def("receiveAsync1", &SignalSlotableWrap::RequestorWrap::receiveAsyncPy1, (bp::arg("replyCallback")),
               "Use receiveAsync instead")
          .def("receiveAsync2", &SignalSlotableWrap::RequestorWrap::receiveAsyncPy2, (bp::arg("replyCallback")),
               "Use receiveAsync instead")
          .def("receiveAsync3", &SignalSlotableWrap::RequestorWrap::receiveAsyncPy3, (bp::arg("replyCallback")),
               "Use receiveAsync instead")
          .def("receiveAsync4", &SignalSlotableWrap::RequestorWrap::receiveAsyncPy4, (bp::arg("replyCallback")),
               "Use receiveAsync instead");

    bp::class_<SignalSlotableWrap::AsyncReplyWrap>("AsyncReply", bp::no_init)
          .def("__call__", &SignalSlotableWrap::AsyncReplyWrap::replyPy0, "Reply slot call without argument")
          .def("__call__", &SignalSlotableWrap::AsyncReplyWrap::replyPy1, (bp::arg("a1")),
               "Reply slot call with one argument")
          .def("__call__", &SignalSlotableWrap::AsyncReplyWrap::replyPy2, (bp::arg("a1"), bp::arg("a2")),
               "Reply slot call with two arguments")
          .def("__call__", &SignalSlotableWrap::AsyncReplyWrap::replyPy3, (bp::arg("a1"), bp::arg("a2"), bp::arg("a3")),
               "Reply slot call with three arguments")
          .def("__call__", &SignalSlotableWrap::AsyncReplyWrap::replyPy4,
               (bp::arg("a1"), bp::arg("a2"), bp::arg("a4"), bp::arg("a4")), "Reply slot call with four arguments")
          .def("error", &SignalSlotableWrap::AsyncReply::error, (bp::arg("message"), bp::arg("details")),
               "Reply failure of slot call stating an error message and details like the\n"
               "stack trace from 'traceback.format_exc()'");

    bp::class_<SignalSlotable, boost::noncopyable>("SignalSlotableIntern")
          .def(bp::init<const std::string&, const karabo::net::Broker::Pointer&>());

    bp::class_<SignalSlotableWrap, boost::shared_ptr<SignalSlotableWrap>, bp::bases<SignalSlotable>,
               boost::noncopyable>("SignalSlotable")
          .def(bp::init<>())
          .def(bp::init<const std::string&>())
          .def(bp::init<const std::string&, const karabo::util::Hash&>())
          .def(bp::init<const std::string&, const karabo::util::Hash&, const int>())
          .def(bp::init<const std::string&, const karabo::util::Hash&, const int, const karabo::util::Hash&>())

          .def("create", &SignalSlotableWrap::create,
               (bp::arg("instanceId"), bp::arg("connectionParameters") = karabo::util::Hash(),
                bp::arg("heartbeatInterval") = 10, bp::arg("instanceInfo") = karabo::util::Hash()),
               "\nUse this factory method to create SignalSlotable object with given 'instanceId', "
               "'connectionParameters', 'heartbeatInterval' and 'instanceInfo'.\n"
               "Example:\n\tss = SignalSlotable.create('a')\n")
          .staticmethod("create")

          .def("start", &SignalSlotableWrap::start)

          .def("getSenderInfo",
               (const boost::shared_ptr<karabo::xms::Slot>& (SignalSlotable::*)(const std::string&))(
                     &SignalSlotable::getSenderInfo),
               bp::arg("slotFunction"), bp::return_value_policy<bp::copy_const_reference>())
          .def("getInstanceId", (bp::object(SignalSlotableWrap::*)()) & SignalSlotableWrap::getInstanceId)
          .def("updateInstanceInfo",
               (void(SignalSlotable::*)(const karabo::util::Hash&, bool))(&SignalSlotable::updateInstanceInfo),
               (bp::arg("update"), bp::arg("remove") = false))
          .def("getInstanceInfo",
               static_cast<karabo::util::Hash (SignalSlotable::*)() const>(&SignalSlotable::getInstanceInfo))
          .def("registerSlotCallGuardHandler", &SignalSlotableWrap::registerSlotCallGuardHandlerPy,
               (bp::arg("handler")))

          .def("registerPerformanceStatisticsHandler", &SignalSlotableWrap::registerPerformanceStatisticsHandlerPy,
               (bp::arg("handler")))

          .def("registerBrokerErrorHandler", &SignalSlotableWrap::registerBrokerErrorHandlerPy, (bp::arg("handler")))

          .def("connect", &SignalSlotableWrap::connectPy,
               (bp::arg("signalInstanceId"), bp::arg("signalFunction"), bp::arg("slotInstanceId"),
                bp::arg("slotFunction")),
               "\nUse this method to connect \"signalFunction\" issued by \"signalInstanceId\" with \"slotFunction\" "
               "belonging to \"slotInstanceId\""
               "\n\nExample:\n\nIf we have to communicate with remote client registered as instance \"b\" and slot "
               "\"onMoin\" ...\n\n\t"
               "ss = SignalSlotable(\"a\")\n\tss.connect(\"\", \"moin\", \"b\", \"onMoin\")\n\tss.emit(\"moin\", 12)\n")

          .def("connect", &SignalSlotableWrap::connectPy_old, (bp::arg("signalFunction"), bp::arg("slotFunction")))

          .def("getAvailableInstances", &SignalSlotableWrap::getAvailableInstancesPy,
               (bp::arg("activateTracking") = false))
          .def("getAvailableSignals", &SignalSlotableWrap::getAvailableSignalsPy, bp::arg("instanceId"))
          .def("getAvailableSlots", &SignalSlotableWrap::getAvailableSlotsPy, bp::arg("instanceId"))

          .def("disconnect", &SignalSlotable::disconnect,
               (bp::arg("signalInstanceId"), bp::arg("signalFunction"), bp::arg("slotInstanceId"),
                bp::arg("slotFunction")))
          .def("getInstanceId", (string const& (SignalSlotable::*)() const)(&SignalSlotable::getInstanceId),
               bp::return_value_policy<bp::copy_const_reference>())

          .def("registerSlot", (&SignalSlotableWrap::registerSlotPy),
               (bp::arg("slotFunction"), bp::arg("slotName") = std::string(), bp::arg("numArgs") = -1),
               "Register a callable as slot function.\n"
               "If slotName empty (default), it is registered under slotFunction.__name__, else under slotName\n"
               "If numArg < 0 (default), try to deduce number of arguments that the function should take\n"
               "   - that is known to work for functions and methods without '*args'. Otherwise use numArgs.")


          .def("registerSignal", &SignalSlotableWrap::registerSignalPy<>, (bp::arg("signalFunction")))
          .def("registerSignal", &SignalSlotableWrap::registerSignalPy<bp::object>,
               (bp::arg("signalFunction"), bp::arg("a1")))
          .def("registerSignal", &SignalSlotableWrap::registerSignalPy<bp::object, bp::object>,
               (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2")))
          .def("registerSignal", &SignalSlotableWrap::registerSignalPy<bp::object, bp::object, bp::object>,
               (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
          .def("registerSignal", &SignalSlotableWrap::registerSignalPy<bp::object, bp::object, bp::object, bp::object>,
               (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

          .def("registerSystemSignal", &SignalSlotableWrap::registerSystemSignalPy<>, (bp::arg("signalFunction")))
          .def("registerSystemSignal", &SignalSlotableWrap::registerSystemSignalPy<bp::object>,
               (bp::arg("signalFunction"), bp::arg("a1")))
          .def("registerSystemSignal", &SignalSlotableWrap::registerSystemSignalPy<bp::object, bp::object>,
               (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2")))
          .def("registerSystemSignal", &SignalSlotableWrap::registerSystemSignalPy<bp::object, bp::object, bp::object>,
               (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
          .def("registerSystemSignal",
               &SignalSlotableWrap::registerSystemSignalPy<bp::object, bp::object, bp::object, bp::object>,
               (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

          .def("emit", &SignalSlotableWrap::emitPy<>, (bp::arg("signalFunction")))
          .def("emit", &SignalSlotableWrap::emitPy<bp::object>, (bp::arg("signalFunction"), bp::arg("a1")))
          .def("emit", &SignalSlotableWrap::emitPy<bp::object, bp::object>,
               (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2")))
          .def("emit", &SignalSlotableWrap::emitPy<bp::object, bp::object, bp::object>,
               (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
          .def("emit", &SignalSlotableWrap::emitPy<bp::object, bp::object, bp::object, bp::object>,
               (bp::arg("signalFunction"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

          .def("call", &SignalSlotableWrap::callPy<>, (bp::arg("instanceId"), bp::arg("slotName")))
          .def("call", &SignalSlotableWrap::callPy<bp::object>,
               (bp::arg("instanceId"), bp::arg("slotName"), bp::arg("a1")))
          .def("call", &SignalSlotableWrap::callPy<bp::object, bp::object>,
               (bp::arg("instanceId"), bp::arg("slotName"), bp::arg("a1"), bp::arg("a2")))
          .def("call", &SignalSlotableWrap::callPy<bp::object, bp::object, bp::object>,
               (bp::arg("instanceId"), bp::arg("slotName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
          .def("call", &SignalSlotableWrap::callPy<bp::object, bp::object, bp::object, bp::object>,
               (bp::arg("instanceId"), bp::arg("slotName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

          .def("request", &SignalSlotableWrap::requestPy<>, (bp::arg("instanceId"), bp::arg("slotName")))
          .def("request", &SignalSlotableWrap::requestPy<bp::object>,
               (bp::arg("instanceId"), bp::arg("slotName"), bp::arg("a1")))
          .def("request", &SignalSlotableWrap::requestPy<bp::object, bp::object>,
               (bp::arg("instanceId"), bp::arg("slotName"), bp::arg("a1"), bp::arg("a2")))
          .def("request", &SignalSlotableWrap::requestPy<bp::object, bp::object, bp::object>,
               (bp::arg("instanceId"), bp::arg("slotName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
          .def("request", &SignalSlotableWrap::requestPy<bp::object, bp::object, bp::object, bp::object>,
               (bp::arg("instanceId"), bp::arg("slotName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

          .def("requestNoWait", &SignalSlotableWrap::requestNoWaitPy<>,
               (bp::arg("requestInstanceId"), bp::arg("requestSlotName"), bp::arg("replyInstanceId"),
                bp::arg("replySlotName")))
          .def("requestNoWait", &SignalSlotableWrap::requestNoWaitPy<bp::object>,
               (bp::arg("requestInstanceId"), bp::arg("requestSlotName"), bp::arg("replyInstanceId"),
                bp::arg("replySlotName"), bp::arg("a1")))
          .def("requestNoWait", &SignalSlotableWrap::requestNoWaitPy<bp::object, bp::object>,
               (bp::arg("requestInstanceId"), bp::arg("requestSlotName"), bp::arg("replyInstanceId"),
                bp::arg("replySlotName"), bp::arg("a1"), bp::arg("a2")))
          .def("requestNoWait", &SignalSlotableWrap::requestNoWaitPy<bp::object, bp::object, bp::object>,
               (bp::arg("requestInstanceId"), bp::arg("requestSlotName"), bp::arg("replyInstanceId"),
                bp::arg("replySlotName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
          .def("requestNoWait", &SignalSlotableWrap::requestNoWaitPy<bp::object, bp::object, bp::object, bp::object>,
               (bp::arg("requestInstanceId"), bp::arg("requestSlotName"), bp::arg("replyInstanceId"),
                bp::arg("replySlotName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

          .def("reply", &SignalSlotableWrap::replyPy<>)
          .def("reply", &SignalSlotableWrap::replyPy<bp::object>, (bp::arg("a1")))
          .def("reply", &SignalSlotableWrap::replyPy<bp::object, bp::object>, (bp::arg("a1"), bp::arg("a2")))
          .def("reply", &SignalSlotableWrap::replyPy<bp::object, bp::object, bp::object>,
               (bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
          .def("reply", &SignalSlotableWrap::replyPy<bp::object, bp::object, bp::object, bp::object>,
               (bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))

          .def("createAsyncReply", &SignalSlotableWrap::createAsyncReply,
               "Create an AsyncReply to postpone the reply of a slot.\n\n"
               "Only call within a slot call - and then take care to use the AsyncReply to\n"
               "either reply or report an error since no automatic reply will happen.")

          .def("createOutputChannel", &SignalSlotableWrap::createOutputChannelPy,
               (bp::arg("channelName"), bp::arg("configuration"), bp::arg("handler") = bp::object()))

          .def("createInputChannel", &SignalSlotableWrap::createInputChannelPy,
               (bp::arg("channelName"), bp::arg("configuration"), bp::arg("onData") = bp::object(),
                bp::arg("onInput") = bp::object(), bp::arg("onEndOfStream") = bp::object(),
                bp::arg("connectionTracker") = bp::object()))

          .def("connectInputChannels", &SignalSlotableWrap::connectInputChannelsPy)

          .def("getOutputChannel", &SignalSlotableWrap::getOutputChannelPy, (bp::arg("channelName")))

          .def("getInputChannel", &SignalSlotableWrap::getInputChannelPy, (bp::arg("channelName")))

          .def("getOutputChannelNames", &SignalSlotable::getOutputChannelNames)

          .def("getInputChannelNames", &SignalSlotableWrap::getInputChannelNamesPy)

          .def("removeInputChannel", &SignalSlotable::removeInputChannel, (bp::arg("channelName")))

          .def("removeOutputChannel", &SignalSlotable::removeOutputChannel, (bp::arg("channelName")))

          .def("registerDataHandler", &SignalSlotableWrap::registerDataHandlerPy,
               (bp::arg("channelName"), bp::arg("handlerPerData") = bp::object()))

          .def("registerInputHandler", &SignalSlotableWrap::registerInputHandlerPy,
               (bp::arg("channelName"), bp::arg("handlerPerInput") = bp::object()))

          .def("registerEndOfStreamHandler", &SignalSlotableWrap::registerEndOfStreamHandlerPy,
               (bp::arg("channelName"), bp::arg("handler") = bp::object()))

          .def("exists", (bp::tuple(SignalSlotableWrap::*)(const std::string&))(&SignalSlotableWrap::existsPy),
               (bp::arg("instanceId")))
          .def("getConnection", &SignalSlotable::getConnection)
          .def("getTopic", &SignalSlotable::getTopic, bp::return_value_policy<bp::copy_const_reference>());
}
