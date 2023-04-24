/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   SignalSlotableWrap.hh
 * Author: <burkhard.heisen@xfel.eu>
 * Contributions: <irina.kozlova@xfel.eu>
 *
 * Created on March 19, 2012, 11:22 AM
 */

#ifndef KARATHON_SIGNALSLOTABLE_HH
#define KARATHON_SIGNALSLOTABLE_HH

#include <boost/function.hpp>
#include <boost/python.hpp>
#include <iostream>
#include <karabo/log/Logger.hh>
#include <karabo/util/ClassInfo.hh>
#include <karabo/xms/SignalSlotable.hh>

#include "PyXmsInputOutputChannel.hh"
#include "ScopedGILRelease.hh"
#include "SignalWrap.hh"
#include "SlotWrap.hh"
#include "Wrapper.hh"

namespace bp = boost::python;

namespace karathon {

    class SignalSlotableWrap : public karabo::xms::SignalSlotable {
       public:
        KARABO_CLASSINFO(SignalSlotableWrap, "SignalSlotableWrap", "1.0")

        class RequestorWrap : public karabo::xms::SignalSlotable::Requestor {
           public:
            explicit RequestorWrap(karabo::xms::SignalSlotable* signalSlotable);

            RequestorWrap timeoutPy(const int& milliseconds);

            template <typename... Args>
            RequestorWrap& requestPy(const std::string& slotInstanceId, const std::string& slotFunction,
                                     const Args&... args) {
                try {
                    auto body = boost::make_shared<karabo::util::Hash>();
                    packPy(*body, args...);
                    ScopedGILRelease nogil;
                    registerRequest(slotInstanceId, prepareRequestHeader(slotInstanceId, slotFunction), body);
                } catch (...) {
                    KARABO_RETHROW
                }
                return *this;
            }

            template <typename... Args>
            RequestorWrap& requestNoWaitPy(const std::string& requestSlotInstanceId,
                                           const std::string& requestSlotFunction,
                                           const std::string replySlotInstanceId, const std::string& replySlotFunction,
                                           const Args&... args) {
                try {
                    auto body = boost::make_shared<karabo::util::Hash>();
                    packPy(*body, args...);
                    ScopedGILRelease nogil;
                    karabo::util::Hash::Pointer header = prepareRequestNoWaitHeader(
                          requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction);
                    registerRequest(requestSlotInstanceId, header, body);
                    sendRequest();
                } catch (...) {
                    KARABO_RETHROW
                }
                return *this;
            }

            void receiveAsyncPy(const bp::object& replyCallback, const bp::object& errorCallback = bp::object(),
                                const bp::object& timeoutMs = bp::object(),
                                const bp::object& numCallbackArgs = bp::object());

            void receiveAsyncPy0(const bp::object& replyCallback);

            void receiveAsyncPy1(const bp::object& replyCallback);

            void receiveAsyncPy2(const bp::object& replyCallback);

            void receiveAsyncPy3(const bp::object& replyCallback);

            void receiveAsyncPy4(const bp::object& replyCallback);

            bp::tuple waitForReply(const int& milliseconds);

           private:
            bp::tuple prepareTuple0(const karabo::util::Hash& body);

            bp::tuple prepareTuple1(const karabo::util::Hash& body);

            bp::tuple prepareTuple2(const karabo::util::Hash& body);

            bp::tuple prepareTuple3(const karabo::util::Hash& body);

            bp::tuple prepareTuple4(const karabo::util::Hash& body);
        };

        class AsyncReplyWrap : public karabo::xms::SignalSlotable::AsyncReply {
           public:
            explicit AsyncReplyWrap(SignalSlotable* signalSlotable);

            void replyPy0() const;

            void replyPy1(const bp::object& a1) const;

            void replyPy2(const bp::object& a1, const bp::object& a2) const;

            void replyPy3(const bp::object& a1, const bp::object& a2, const bp::object& a3) const;

            void replyPy4(const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object& a4) const;
        };

        AsyncReplyWrap createAsyncReply() {
            return SignalSlotableWrap::AsyncReplyWrap(this);
        }

       public:
        SignalSlotableWrap(const std::string& instanceId = generateInstanceId<SignalSlotable>(),
                           const karabo::util::Hash& connectionParameters = karabo::util::Hash(),
                           int heartbeatInterval = 10, const karabo::util::Hash& instanceInfo = karabo::util::Hash());

        virtual ~SignalSlotableWrap();

        static boost::shared_ptr<SignalSlotableWrap> create(
              const std::string& instanceId = generateInstanceId<SignalSlotable>(),
              const karabo::util::Hash& connectionParameters = karabo::util::Hash(), int heartbeatInterval = 10,
              const karabo::util::Hash& instanceInfo = karabo::util::Hash()) {
            return boost::shared_ptr<SignalSlotableWrap>(
                  new SignalSlotableWrap(instanceId, connectionParameters, heartbeatInterval, instanceInfo));
        }

        void start() {
            ScopedGILRelease nogil;
            try {
                karabo::xms::SignalSlotable::start();
            } catch (const std::exception& e) {
                // Make sure that we get something in the log file and not only in standard output/error
                KARABO_LOG_FRAMEWORK_ERROR << e.what();
                KARABO_RETHROW;
            }
        }

        bp::object getInstanceId() {
            return bp::object(SignalSlotable::getInstanceId());
        }

        bp::tuple existsPy(const std::string& instanceId) {
            std::pair<bool, std::string> result;
            {
                ScopedGILRelease nogil;
                result = exists(instanceId);
            }
            return bp::make_tuple(result.first, bp::object(result.second));
        }

        karabo::util::Hash getAvailableInstancesPy(bool activateTracking) {
            return this->getAvailableInstances(activateTracking);
        }

        bp::object getAvailableSignalsPy(const std::string& instanceId) {
            return Wrapper::fromStdVectorToPyList(this->getAvailableSignals(instanceId));
        }

        bp::object getAvailableSlotsPy(const std::string& instanceId) {
            return Wrapper::fromStdVectorToPyList(this->getAvailableSlots(instanceId));
        }

        void registerSlotPy(const bp::object& slotFunction, std::string slotName, int numArg);

        template <typename... Args>
        void registerSignalPy(const std::string& funcName, const Args&... args) {
            // Arguments are ignored, but required to partially deduce the signature of the signal in Python:
            // All args will always be bp::object, but at least the number of arguments defines the signal signature
            registerSignal<Args...>(funcName);
        }

        template <typename... Args>
        void registerSystemSignalPy(const std::string& funcName, const Args&... args) {
            // Arguments are ignored, but required to partially deduce the signature of the signal in Python:
            // All args will always be bp::object, but at least the number of arguments defines the signal signature
            registerSystemSignal<Args...>(funcName);
        }

        template <typename... Args>
        SignalSlotableWrap::RequestorWrap requestPy(std::string instanceId, const std::string& functionName,
                                                    const Args&... args) {
            if (instanceId.empty()) instanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestPy(instanceId, functionName, args...);
        }

        template <typename... Args>
        SignalSlotableWrap::RequestorWrap requestNoWaitPy(std::string requestSlotInstanceId,
                                                          const std::string& requestSlotFunction,
                                                          std::string replySlotInstanceId,
                                                          const std::string& replySlotFunction, const Args&... args) {
            if (requestSlotInstanceId.empty()) requestSlotInstanceId = m_instanceId;
            if (replySlotInstanceId.empty()) replySlotInstanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestNoWaitPy(
                  requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction, args...);
        }

        template <typename... Args>
        void emitPy(const std::string& signalFunction, const Args&... args) {
            auto s = getSignal(signalFunction);
            if (s) {
                auto hash = boost::make_shared<karabo::util::Hash>();
                packPy(*hash, args...);
                s->emit<Args...>(hash);
            }
        }

        template <typename... Args>
        void callPy(const std::string& instanceId, const std::string& functionName, const Args&... args) const {
            auto body = boost::make_shared<karabo::util::Hash>();
            packPy(*body, args...);
            const std::string& id = (instanceId.empty() ? m_instanceId : instanceId);
            auto header = prepareCallHeader(id, functionName);
            doSendMessage(id, header, body, KARABO_SYS_PRIO, KARABO_SYS_TTL);
        }

        template <typename... Args>
        void replyPy(const Args&... args) {
            auto reply(boost::make_shared<karabo::util::Hash>());
            packPy(*reply, args...);
            registerReply(reply);
        }

        bool connectPy(const std::string& signalInstanceId, const std::string& signalFunction,
                       const std::string& slotInstanceId, const std::string& slotFunction) {
            ScopedGILRelease nogil;
            return this->connect(signalInstanceId, signalFunction, slotInstanceId, slotFunction);
        }

        bool connectPy_old(const std::string& signalFunction, const std::string& slotFunction) {
            ScopedGILRelease nogil;
            return this->connect(signalFunction, slotFunction);
        }

        void registerSlotCallGuardHandlerPy(const bp::object& handler) {
            registerSlotCallGuardHandler(
                  HandlerWrap<const std::string&, const std::string&>(handler, "slot call guard"));
        }

        void registerPerformanceStatisticsHandlerPy(const bp::object& handler) {
            registerPerformanceStatisticsHandler(
                  HandlerWrap<const karabo::util::Hash::Pointer&>(handler, "performance measurement"));
        }

        void registerBrokerErrorHandlerPy(const bp::object& handler) {
            registerBrokerErrorHandler(HandlerWrap<const std::string&>(handler, "broker error"));
        }

        karabo::xms::OutputChannel::Pointer createOutputChannelPy(
              const std::string& channelName, const karabo::util::Hash& config,
              const bp::object& onOutputPossibleHandler = bp::object()) {
            using Wrap = HandlerWrap<const karabo::xms::OutputChannel::Pointer&>;
            return createOutputChannel(channelName, config, Wrap(onOutputPossibleHandler, "IOEvent"));
        }

        karabo::xms::InputChannel::Pointer createInputChannelPy(const std::string& channelName,
                                                                const karabo::util::Hash& config,
                                                                const bp::object& onDataHandler = bp::object(),
                                                                const bp::object& onInputHandler = bp::object(),
                                                                const bp::object& onEndOfStreamHandler = bp::object(),
                                                                const bp::object& connectionTracker = bp::object());

        void connectInputChannelsPy() {
            ScopedGILRelease nogil;
            this->connectInputChannels(boost::system::error_code());
        }

        bp::object getOutputChannelPy(const std::string& name) {
            return bp::object(getOutputChannel(name));
        }

        bp::object getInputChannelPy(const std::string& name) {
            return bp::object(getInputChannel(name));
        }

        bp::list getInputChannelNamesPy() {
            bp::list result;
            for (const auto& inputNameChannel : getInputChannels()) {
                result.append(inputNameChannel.first);
            }
            return result;
        }

        void registerDataHandlerPy(const std::string& channelName, const bp::object& handler) {
            registerDataHandler(channelName, InputChannelWrap::DataHandlerWrap(handler, "data"));
        }

        void registerInputHandlerPy(const std::string& channelName, const bp::object& handler) {
            registerInputHandler(channelName, HandlerWrap<const karabo::xms::InputChannel::Pointer&>(handler, "input"));
        }

        void registerEndOfStreamHandlerPy(const std::string& channelName, const bp::object& handler) {
            registerEndOfStreamHandler(channelName,
                                       HandlerWrap<const karabo::xms::InputChannel::Pointer&>(handler, "EOS"));
        }
    };
} // namespace karathon

#endif /* KARATHON_SIGNALSLOTABLE_HH */
