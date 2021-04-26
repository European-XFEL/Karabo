/* 
 * File:   SignalSlotableWrap.hh
 * Author: <burkhard.heisen@xfel.eu>
 * Contributions: <irina.kozlova@xfel.eu>
 *
 * Created on March 19, 2012, 11:22 AM
 */

#ifndef KARATHON_SIGNALSLOTABLE_HH
#define	KARATHON_SIGNALSLOTABLE_HH

#include <karabo/util/ClassInfo.hh>
#include <karabo/log/Logger.hh>
#include <karabo/xms/SignalSlotable.hh>

#include <boost/python.hpp>
#include <boost/function.hpp>

#include <iostream>

#include "SignalWrap.hh"
#include "SlotWrap.hh"
#include "ScopedGILRelease.hh"
#include "Wrapper.hh"
#include "PyXmsInputOutputChannel.hh"

namespace bp = boost::python;

namespace karathon {

    class SignalSlotableWrap : public karabo::xms::SignalSlotable {

    public:
        KARABO_CLASSINFO(SignalSlotableWrap, "SignalSlotableWrap", "1.0")

        class RequestorWrap : public karabo::xms::SignalSlotable::Requestor {

        public:

            explicit RequestorWrap(karabo::xms::SignalSlotable* signalSlotable);

            RequestorWrap timeoutPy(const int& milliseconds);

            template<typename ...Args>
            RequestorWrap& requestPy(const std::string& slotInstanceId,
                                     const std::string& slotFunction,
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

            template<typename ...Args>
            RequestorWrap& requestNoWaitPy(const std::string& requestSlotInstanceId,
                                           const std::string& requestSlotFunction,
                                           const std::string replySlotInstanceId,
                                           const std::string& replySlotFunction,
                                           const Args&... args) {
                try {
                    auto body = boost::make_shared<karabo::util::Hash>();
                    packPy(*body, args...);
                    ScopedGILRelease nogil;
                    karabo::util::Hash::Pointer header = prepareRequestNoWaitHeader(requestSlotInstanceId,
                                                                                    requestSlotFunction,
                                                                                    replySlotInstanceId,
                                                                                    replySlotFunction);
                    registerRequest(requestSlotInstanceId, header, body);
                    sendRequest();
                } catch (...) {
                    KARABO_RETHROW
                }
                return *this;
            }

            void receiveAsyncPy0(const bp::object& replyCallback);

            void proxyReceiveAsync0(const bp::object& replyCallback);

            void receiveAsyncPy1(const bp::object& replyCallback);

            void proxyReceiveAsync1(const bp::object& replyCallback, const boost::any& a1);

            void receiveAsyncPy2(const bp::object& replyCallback);

            void proxyReceiveAsync2(const bp::object& replyCallback, const boost::any& a1, const boost::any& a2);

            void receiveAsyncPy3(const bp::object& replyCallback);

            void proxyReceiveAsync3(const bp::object& replyCallback, const boost::any& a1, const boost::any& a2, const boost::any& a3);

            void receiveAsyncPy4(const bp::object& replyCallback);

            void proxyReceiveAsync4(const bp::object& replyCallback,
                                    const boost::any& a1, const boost::any& a2, const boost::any& a3, const boost::any& a4);

            bp::tuple receivePy0();

            bp::tuple receivePy1();

            bp::tuple receivePy2();

            bp::tuple receivePy3();

            bp::tuple receivePy4();

            bp::tuple waitForReply(const int& milliseconds);

            bp::tuple prepareTuple0(const karabo::util::Hash & body);

            bp::tuple prepareTuple1(const karabo::util::Hash & body);

            bp::tuple prepareTuple2(const karabo::util::Hash & body);

            bp::tuple prepareTuple3(const karabo::util::Hash & body);

            bp::tuple prepareTuple4(const karabo::util::Hash & body);
        };

    public:

        SignalSlotableWrap(const std::string& instanceId = generateInstanceId<SignalSlotable>(),
                           const karabo::util::Hash& connectionParameters = karabo::util::Hash(),
                           int heartbeatInterval = 10,
                           const karabo::util::Hash& instanceInfo = karabo::util::Hash());

        virtual ~SignalSlotableWrap();

        static boost::shared_ptr<SignalSlotableWrap>
        create(const std::string& instanceId = generateInstanceId<SignalSlotable>(),
               const karabo::util::Hash& connectionParameters = karabo::util::Hash(),
               int heartbeatInterval = 10,
               const karabo::util::Hash& instanceInfo = karabo::util::Hash()) {
            return boost::shared_ptr<SignalSlotableWrap>(new SignalSlotableWrap(instanceId,
                                                                                connectionParameters,
                                                                                heartbeatInterval,
                                                                                instanceInfo));
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

        template<typename ...Args>
        void registerSignalPy(const std::string& funcName, const Args&... args) {
            // Arguments are ignored, but required to partially deduce the signature of the signal in Python:
            // All args will always be bp::object, but at least the number of arguments defines the signal signature
            registerSignal < Args...>(funcName);
        }

        template<typename ...Args>
        void registerSystemSignalPy(const std::string& funcName, const Args&... args) {
            // Arguments are ignored, but required to partially deduce the signature of the signal in Python:
            // All args will always be bp::object, but at least the number of arguments defines the signal signature
            registerSystemSignal < Args...>(funcName);
        }

        template<typename ...Args>
        SignalSlotableWrap::RequestorWrap requestPy(std::string instanceId,
                                                    const std::string& functionName,
                                                    const Args&... args) {
            if (instanceId.empty()) instanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestPy(instanceId, functionName, args...);
        }

        template<typename ...Args>
        SignalSlotableWrap::RequestorWrap requestNoWaitPy(std::string requestSlotInstanceId,
                                                          const std::string& requestSlotFunction,
                                                          std::string replySlotInstanceId,
                                                          const std::string& replySlotFunction,
                                                          const Args&... args) {

            if (requestSlotInstanceId.empty()) requestSlotInstanceId = m_instanceId;
            if (replySlotInstanceId.empty()) replySlotInstanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestNoWaitPy(requestSlotInstanceId, requestSlotFunction,
                                                                           replySlotInstanceId, replySlotFunction,
                                                                           args...);
        }

        template<typename ...Args>
        void emitPy(const std::string& signalFunction, const Args&... args) {
            auto s = getSignal(signalFunction);
            if (s) {
                auto hash = boost::make_shared<karabo::util::Hash>();
                packPy(*hash, args...);
                s->emit < Args...>(hash);
            }
        }

        template<typename ...Args>
        void callPy(const std::string& instanceId, const std::string& functionName, const Args&... args) const {
            auto body = boost::make_shared<karabo::util::Hash>();
            packPy(*body, args...);
            const std::string& id = (instanceId.empty() ? m_instanceId : instanceId);
            auto header = prepareCallHeader(id, functionName);
            doSendMessage(id, header, body, KARABO_SYS_PRIO, KARABO_SYS_TTL);
        }

        template <typename ...Args>
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

        void registerInstanceNewHandlerPy(const bp::object& handler) {
            registerInstanceNewHandler(boost::bind(&SignalSlotableWrap::proxyInstanceNewHandler,
                                                   this, handler, _1, _2));
        }

        void registerInstanceGoneHandlerPy(const bp::object& handler) {
            registerInstanceGoneHandler(boost::bind(&SignalSlotableWrap::proxyInstanceGoneHandler,
                                                    this, handler, _1, _2));
        }

        void registerSlotCallGuardHandlerPy(const bp::object& handler) {
            registerSlotCallGuardHandler(boost::bind(&SignalSlotableWrap::proxySlotCallGuardHandler, this, handler, _1, _2));
        }

        void registerPerformanceStatisticsHandlerPy(const bp::object& handler) {
            registerPerformanceStatisticsHandler(boost::bind(&SignalSlotableWrap::proxyUpdatePerformanceStatisticsHandler,
                                                             this, handler, _1));
        }

        void registerBrokerErrorHandlerPy(const bp::object& handler) {
            registerBrokerErrorHandler(boost::bind(&SignalSlotableWrap::proxyBrokerErrorHandler, this, handler, _1));
        }

        karabo::xms::OutputChannel::Pointer
        createOutputChannelPy(const std::string& channelName,
                              const karabo::util::Hash& config,
                              const bp::object& onOutputPossibleHandler = bp::object()) {
            return createOutputChannel(channelName,
                                       config,
                                       boost::bind(&OutputChannelWrap::proxyIOEventHandler, onOutputPossibleHandler, _1));
        }

        karabo::xms::InputChannel::Pointer
        createInputChannelPy(const std::string& channelName,
                             const karabo::util::Hash& config,
                             const bp::object& onDataHandler = bp::object(),
                             const bp::object& onInputHandler = bp::object(),
                             const bp::object& onEndOfStreamHandler = bp::object());

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
            registerDataHandler(channelName, boost::bind(&InputChannelWrap::proxyDataHandler, handler, _1, _2));
        }

        void registerInputHandlerPy(const std::string& channelName, const bp::object& handler) {
            registerInputHandler(channelName, boost::bind(&InputChannelWrap::proxyInputHandler, handler, _1));
        }

        void registerEndOfStreamHandlerPy(const std::string& channelName, const bp::object& handler) {
            registerEndOfStreamHandler(channelName, boost::bind(&InputChannelWrap::proxyEndOfStreamEventHandler, handler, _1));
        }

    private:

        void proxyInstanceNewHandler(const bp::object& handler, const std::string& instanceId, const karabo::util::Hash& instanceInfo);

        void proxyInstanceGoneHandler(const bp::object& handler, const std::string& instanceId, const karabo::util::Hash& instanceInfo);

        void proxyExceptionHandler(const bp::object& handler, const karabo::util::Exception& e);

        void proxySlotCallGuardHandler(const bp::object&, const std::string&, const std::string&);

        void proxyUpdatePerformanceStatisticsHandler(const bp::object&, const karabo::util::Hash::Pointer&);

        void proxyBrokerErrorHandler(const bp::object&, const std::string&);

    };
}

#endif  /* KARATHON_SIGNALSLOTABLE_HH */

