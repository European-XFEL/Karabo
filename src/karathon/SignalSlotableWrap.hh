/* 
 * File:   SignalSlotableWrap.hh
 * Author: <burkhard.heisen@xfel.eu>
 * Contributions: <irina.kozlova@xfel.eu>
 *
 * Created on March 19, 2012, 11:22 AM
 */

#ifndef KARATHON_SIGNALSLOTABLE_HH
#define	KARATHON_SIGNALSLOTABLE_HH

#include <boost/python.hpp>
#include <boost/function.hpp>
#include <karabo/xms.hpp>
#include <iostream>
#include "SignalWrap.hh"
#include "SlotWrap.hh"
#include "ScopedGILRelease.hh"
#include "Wrapper.hh"

namespace bp = boost::python;

namespace karathon {

    class SignalSlotableWrap : public karabo::xms::SignalSlotable {

    public:

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
                    sendRequest(slotInstanceId, prepareHeader(slotInstanceId, slotFunction), body);
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
                    karabo::util::Hash::Pointer header = prepareHeaderNoWait(requestSlotInstanceId,
                                                                             requestSlotFunction,
                                                                             replySlotInstanceId,
                                                                             replySlotFunction);
                    sendRequest(requestSlotInstanceId, header, body);
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
                           const std::string& connectionType = "Jms",
                           const karabo::util::Hash& connectionParameters = karabo::util::Hash(),
                           const bool autostartEventLoop = true,
                           int heartbeatInterval = 10);

        virtual ~SignalSlotableWrap();

        static boost::shared_ptr<SignalSlotableWrap> create(const std::string& instanceId = generateInstanceId<SignalSlotable>(),
                                                            const std::string& connectionType = "Jms",
                                                            const karabo::util::Hash& connectionParameters = karabo::util::Hash(),
                                                            bool autostart = false,
                                                            int heartbeatInterval = 10) {
            return boost::shared_ptr<SignalSlotableWrap>(new SignalSlotableWrap(instanceId, connectionType, connectionParameters, autostart, heartbeatInterval));
        }

        void runEventLoop(int emitHeartbeat = 10, const karabo::util::Hash& info = karabo::util::Hash()) {
            ScopedGILRelease nogil;
            karabo::xms::SignalSlotable::runEventLoop(emitHeartbeat, info);
        }

        bp::object ensureOwnInstanceIdUnique() {
            bool result = false;
            {
                ScopedGILRelease nogil;
                result = karabo::xms::SignalSlotable::ensureOwnInstanceIdUnique();
            }
            return bp::object(result);
        }

        void stopEventLoop() {
            ScopedGILRelease nogil;
            karabo::xms::SignalSlotable::stopEventLoop();
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

        void registerSlotPy(const bp::object& slotFunction);

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
                s->emit<Args...>(hash);
            }
        }

        template<typename ...Args>
        void callPy(const std::string& instanceId, const std::string& functionName, const Args&... args) const {
            auto body = boost::make_shared<karabo::util::Hash>();
            packPy(*body, args...);
            auto header = prepareCallHeader(instanceId, functionName);
            const std::string& id = (instanceId.empty() ? m_instanceId : instanceId);
            doSendMessage(id, header, body, KARABO_SYS_PRIO, KARABO_SYS_TTL);
        }

        template <typename ...Args>
        void replyPy(const Args&... args) {
            karabo::util::Hash reply;
            packPy(reply, args...);
            registerReply(reply);
        }

        bp::object getBrokerHost() {
            return bp::object(getConnection()->getBrokerHostname() + ":" + karabo::util::toString(getConnection()->getBrokerPort()));
        }

        bp::object getBrokerTopic() {
            return bp::object(getConnection()->getBrokerTopic());
        }

        bp::object getBrokerHosts() {
            return Wrapper::fromStdVectorToPyList<std::string>(getConnection()->getBrokerHosts());
        }

        void registerInstanceNotAvailableHandlerPy(const bp::object& handler) {
            registerInstanceNotAvailableHandler(boost::bind(&SignalSlotableWrap::proxyInstanceNotAvailableHandler,
                                                            this, handler, _1, _2));
        }

        void registerInstanceAvailableAgainHandlerPy(const bp::object& handler) {
            registerInstanceAvailableAgainHandler(boost::bind(&SignalSlotableWrap::proxyInstanceAvailableAgainHandler,
                                                              this, handler, _1, _2));
        }

        void registerExceptionHandlerPy(const bp::object& handler) {
            registerExceptionHandler(boost::bind(&SignalSlotableWrap::proxyExceptionHandler, this, handler, _1));
        }    

        void registerSlotCallGuardHandlerPy(const bp::object& handler) {
            registerSlotCallGuardHandler(boost::bind(&SignalSlotableWrap::proxySlotCallGuardHandler, this, handler, _1));
        }

        void registerPerformanceStatisticsHandlerPy(const bp::object& handler) {
            registerPerformanceStatisticsHandler(boost::bind(&SignalSlotableWrap::proxyUpdatePerformanceStatisticsHandler,
                                                             this, handler, _1, _2, _3, _4, _5));
        }

        karabo::xms::OutputChannel::Pointer
        createOutputChannelPy(const std::string& channelName,
                              const karabo::util::Hash& config,
                              const bp::object& onOutputPossibleHandler = bp::object()) {
            // Should we do same trick concerning bp::object as in createInputChannelPy?
            return createOutputChannel(channelName,
                                       config,
                                       boost::bind(&SignalSlotableWrap::proxyOnOutputPossibleHandler,
                                                   this, onOutputPossibleHandler, _1));
        }

        karabo::xms::InputChannel::Pointer
        createInputChannelPy(const std::string& channelName,
                             const karabo::util::Hash& config,
                             const bp::object& onDataHandler = bp::object(),
                             const bp::object& onInputHandler = bp::object(),
                             const bp::object& onEndOfStreamHandler = bp::object());

        void connectInputChannelsPy() {
            ScopedGILRelease nogil;
            this->connectInputChannels();
        }

        bp::object getOutputChannelPy(const std::string& name) {
            return bp::object(getOutputChannel(name));
        }

        bp::object getInputChannelPy(const std::string& name) {
            return bp::object(getInputChannel(name));
        }

        void registerDataHandlerPy(const std::string& channelName, const bp::object& handler) {
            registerDataHandler(channelName, boost::bind(&SignalSlotableWrap::proxyOnDataAvailableHandler, this, handler, _1));
        }

        void registerInputHandlerPy(const std::string& channelName, const bp::object& handler) {
            registerInputHandler(channelName, boost::bind(&SignalSlotableWrap::proxyOnInputAvailableHandler, this, handler, _1));
        }

        void registerEndOfStreamHandlerPy(const std::string& channelName, const bp::object& handler) {
            registerEndOfStreamHandler(channelName, boost::bind(&SignalSlotableWrap::proxyOnEndOfStreamEventHandler, this, handler, _1));
        }

        void registerLastCommandHandlerPy(const bp::object& handler) {
            registerLastCommandHandler(boost::bind(&SignalSlotableWrap::proxyLastCommandHandler, this, handler, _1));
        }

    private:

        void proxyInstanceNotAvailableHandler(const bp::object& handler, const std::string& instanceId, const karabo::util::Hash& instanceInfo);

        void proxyInstanceAvailableAgainHandler(const bp::object& handler, const std::string& instanceId, const karabo::util::Hash& instanceInfo);

        void proxyExceptionHandler(const bp::object& handler, const karabo::util::Exception& e);       

        bool proxySlotCallGuardHandler(const bp::object&, const std::string&);

        void proxyUpdatePerformanceStatisticsHandler(const bp::object&, float, unsigned int, float, unsigned int, unsigned int);

        void proxyOnOutputPossibleHandler(const bp::object& handler, const karabo::xms::OutputChannel::Pointer& channel);

        void proxyOnInputAvailableHandler(const bp::object& handler, const karabo::xms::InputChannel::Pointer& channel);

        void proxyOnDataAvailableHandler(const bp::object& handler, const karabo::util::Hash::Pointer& data);

        void proxyOnEndOfStreamEventHandler(const bp::object& handler, const karabo::xms::InputChannel::Pointer& channel);

        void proxyLastCommandHandler(const bp::object& handler, const std::string& slotFunction);

    private: // members
        boost::thread m_eventLoop;
    };
}

#endif  /* KARATHON_SIGNALSLOTABLE_HH */

