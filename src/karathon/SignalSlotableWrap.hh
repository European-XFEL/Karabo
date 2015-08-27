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

            RequestorWrap& requestPy(const std::string& slotInstanceId,
                                     const std::string& slotFunction);

            RequestorWrap& requestPy(const std::string& slotInstanceId,
                                     const std::string& slotFunction,
                                     const bp::object& a1);

            RequestorWrap& requestPy(const std::string& slotInstanceId,
                                     const std::string& slotFunction,
                                     const bp::object& a1,
                                     const bp::object& a2);

            RequestorWrap& requestPy(const std::string& slotInstanceId,
                                     const std::string& slotFunction,
                                     const bp::object& a1,
                                     const bp::object& a2,
                                     const bp::object& a3);

            RequestorWrap& requestPy(const std::string& slotInstanceId,
                                     const std::string& slotFunction,
                                     const bp::object& a1,
                                     const bp::object& a2,
                                     const bp::object& a3,
                                     const bp::object& a4);

            RequestorWrap& requestNoWaitPy(const std::string& requestSlotInstanceId,
                                           const std::string& requestSlotFunction,
                                           const std::string replySlotInstanceId,
                                           const std::string& replySlotFunction);

            RequestorWrap& requestNoWaitPy(const std::string& requestSlotInstanceId,
                                           const std::string& requestSlotFunction,
                                           const std::string replySlotInstanceId,
                                           const std::string& replySlotFunction,
                                           const bp::object& a1);

            RequestorWrap & requestNoWaitPy(const std::string& requestSlotInstanceId,
                                            const std::string& requestSlotFunction,
                                            const std::string replySlotInstanceId,
                                            const std::string& replySlotFunction,
                                            const bp::object& a1,
                                            const bp::object& a2);

            RequestorWrap & requestNoWaitPy(const std::string& requestSlotInstanceId,
                                            const std::string& requestSlotFunction,
                                            const std::string replySlotInstanceId,
                                            const std::string& replySlotFunction,
                                            const bp::object& a1,
                                            const bp::object& a2,
                                            const bp::object& a3);

            RequestorWrap & requestNoWaitPy(const std::string& requestSlotInstanceId,
                                            const std::string& requestSlotFunction,
                                            const std::string replySlotInstanceId,
                                            const std::string& replySlotFunction,
                                            const bp::object& a1,
                                            const bp::object& a2,
                                            const bp::object& a3,
                                            const bp::object& a4);

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

        void registerSlotPy(const bp::object& slotFunction, const SlotType& slotType = LOCAL);

        void registerSignalPy0(const std::string& funcName) {
            this->registerSignal(funcName);
        }

        void registerSignalPy1(const std::string& funcName, const bp::object&) {
            boost::shared_ptr<karathon::SignalWrap> s(new karathon::SignalWrap(this, m_producerChannel, m_instanceId, funcName));
            boost::function<void (const bp::object&) > f(boost::bind(&karathon::SignalWrap::emitPy1, s, _1));
            storeSignal(funcName, s, f);
        }

        void registerSignalPy2(const std::string& funcName, const bp::object&, const bp::object&) {
            boost::shared_ptr<karathon::SignalWrap> s(new karathon::SignalWrap(this, m_producerChannel, m_instanceId, funcName));
            boost::function<void (const bp::object&, const bp::object&) > f(boost::bind(&karathon::SignalWrap::emitPy2, s, _1, _2));
            storeSignal(funcName, s, f);
        }

        void registerSignalPy3(const std::string& funcName, const bp::object&, const bp::object&, const bp::object&) {
            boost::shared_ptr<karathon::SignalWrap> s(new karathon::SignalWrap(this, m_producerChannel, m_instanceId, funcName));
            boost::function<void (const bp::object&, const bp::object&, const bp::object&) > f(boost::bind(&karathon::SignalWrap::emitPy3, s, _1, _2, _3));
            storeSignal(funcName, s, f);
        }

        void registerSignalPy4(const std::string& funcName, const bp::object&, const bp::object&, const bp::object&, const bp::object&) {
            boost::shared_ptr<karathon::SignalWrap> s(new karathon::SignalWrap(this, m_producerChannel, m_instanceId, funcName));
            boost::function<void (const bp::object&, const bp::object&, const bp::object&, const bp::object&) > f(boost::bind(&karathon::SignalWrap::emitPy4, s, _1, _2, _3, _4));
            storeSignal(funcName, s, f);
        }

        //        SignalSlotableWrap::RequestorWrap timeoutPy(const int& milliseconds) {
        //            return SignalSlotableWrap::RequestorWrap(this).timeoutPy0(milliseconds);
        //        }

        SignalSlotableWrap::RequestorWrap requestPy0(std::string instanceId, const std::string& functionName) {
            if (instanceId.empty()) instanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestPy(instanceId, functionName);
        }

        SignalSlotableWrap::RequestorWrap requestNoWaitPy0(std::string requestSlotInstanceId,
                                                           const std::string& requestSlotFunction,
                                                           std::string replySlotInstanceId,
                                                           const std::string& replySlotFunction) {

            if (requestSlotInstanceId.empty()) requestSlotInstanceId = m_instanceId;
            if (replySlotInstanceId.empty()) replySlotInstanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestNoWaitPy(requestSlotInstanceId, requestSlotFunction,
                                                                           replySlotInstanceId, replySlotFunction);
        }

        void emitPy1(const std::string& signalFunction, const bp::object& a1) {
            this->emit(signalFunction, a1);
        }

        void callPy1(std::string instanceId, const std::string& functionName, const bp::object& a1) const {
            SignalWrap s(this, m_producerChannel, m_instanceId, "__call__");
            if (instanceId.empty()) instanceId = m_instanceId;
            s.registerSlot(instanceId, functionName);
            s.emitPy1(a1);
        }

        SignalSlotableWrap::RequestorWrap requestPy1(std::string instanceId, const std::string& functionName, const bp::object& a1) {
            if (instanceId.empty()) instanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestPy(instanceId, functionName, a1);
        }

        SignalSlotableWrap::RequestorWrap requestNoWaitPy1(std::string requestSlotInstanceId,
                                                           const std::string& requestSlotFunction,
                                                           std::string replySlotInstanceId,
                                                           const std::string& replySlotFunction, const bp::object& a1) {

            if (requestSlotInstanceId.empty()) requestSlotInstanceId = m_instanceId;
            if (replySlotInstanceId.empty()) replySlotInstanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestNoWaitPy(requestSlotInstanceId, requestSlotFunction,
                                                                           replySlotInstanceId, replySlotFunction,
                                                                           a1);
        }

        void emitPy2(const std::string& signalFunction, const bp::object& a1, const bp::object& a2) {
            this->emit(signalFunction, a1, a2);
        }

        void callPy2(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2) const {
            SignalWrap s(this, m_producerChannel, m_instanceId, "__call__");
            if (instanceId.empty()) instanceId = m_instanceId;
            s.registerSlot(instanceId, functionName);
            s.emitPy2(a1, a2);
        }

        SignalSlotableWrap::RequestorWrap requestPy2(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2) {
            if (instanceId.empty()) instanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestPy(instanceId, functionName, a1, a2);
        }

        SignalSlotableWrap::RequestorWrap requestNoWaitPy2(std::string requestSlotInstanceId,
                                                           const std::string& requestSlotFunction,
                                                           std::string replySlotInstanceId,
                                                           const std::string& replySlotFunction,
                                                           const bp::object& a1, const bp::object& a2) {

            if (requestSlotInstanceId.empty()) requestSlotInstanceId = m_instanceId;
            if (replySlotInstanceId.empty()) replySlotInstanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestNoWaitPy(requestSlotInstanceId, requestSlotFunction,
                                                                           replySlotInstanceId, replySlotFunction,
                                                                           a1, a2);
        }

        void emitPy3(const std::string& signalFunction, const bp::object& a1, const bp::object& a2, const bp::object& a3) {
            this->emit(signalFunction, a1, a2, a3);
        }

        void callPy3(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2, const bp::object& a3) const {
            SignalWrap s(this, m_producerChannel, m_instanceId, "__call__");
            if (instanceId.empty()) instanceId = m_instanceId;
            s.registerSlot(instanceId, functionName);
            s.emitPy3(a1, a2, a3);
        }

        SignalSlotableWrap::RequestorWrap requestPy3(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2, const bp::object& a3) {
            if (instanceId.empty()) instanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestPy(instanceId, functionName, a1, a2, a3);
        }

        SignalSlotableWrap::RequestorWrap requestNoWaitPy3(std::string requestSlotInstanceId,
                                                           const std::string& requestSlotFunction,
                                                           std::string replySlotInstanceId,
                                                           const std::string& replySlotFunction,
                                                           const bp::object& a1, const bp::object& a2, const bp::object& a3) {

            if (requestSlotInstanceId.empty()) requestSlotInstanceId = m_instanceId;
            if (replySlotInstanceId.empty()) replySlotInstanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestNoWaitPy(requestSlotInstanceId, requestSlotFunction,
                                                                           replySlotInstanceId, replySlotFunction,
                                                                           a1, a2, a3);
        }

        void emitPy4(const std::string& signalFunction, const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object& a4) {
            this->emit(signalFunction, a1, a2, a3, a4);
        }

        void callPy4(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object& a4) const {
            SignalWrap s(this, m_producerChannel, m_instanceId, "__call__");
            if (instanceId.empty()) instanceId = m_instanceId;
            s.registerSlot(instanceId, functionName);
            s.emitPy4(a1, a2, a3, a4);
        }

        SignalSlotableWrap::RequestorWrap requestPy4(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object& a4) {
            if (instanceId.empty()) instanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestPy(instanceId, functionName, a1, a2, a3, a4);
        }

        SignalSlotableWrap::RequestorWrap requestNoWaitPy4(std::string requestSlotInstanceId,
                                                           const std::string& requestSlotFunction,
                                                           std::string replySlotInstanceId,
                                                           const std::string& replySlotFunction,
                                                           const bp::object& a1, const bp::object& a2,
                                                           const bp::object& a3, const bp::object& a4) {

            if (requestSlotInstanceId.empty()) requestSlotInstanceId = m_instanceId;
            if (replySlotInstanceId.empty()) replySlotInstanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestNoWaitPy(requestSlotInstanceId, requestSlotFunction,
                                                                           replySlotInstanceId, replySlotFunction,
                                                                           a1, a2, a3, a4);
        }

        //        void receiveAsyncPy0(const bp::object& replyCallback) {
        //            SignalSlotableWrap::RequestorWrap(this).receiveAsyncPy(replyCallback);
        //        }
        //
        //        void receiveAsyncPy1(const bp::object& replyCallback) {
        //            SignalSlotableWrap::RequestorWrap(this).receiveAsyncPy1(replyCallback);
        //        }
        //
        //        void receiveAsyncPy2(const bp::object& replyCallback) {
        //            SignalSlotableWrap::RequestorWrap(this).receiveAsyncPy2(replyCallback);
        //        }
        //
        //        void receiveAsyncPy3(const bp::object& replyCallback) {
        //            SignalSlotableWrap::RequestorWrap(this).receiveAsyncPy3(replyCallback);
        //        }
        //
        //        void receivePy() {
        //            SignalSlotableWrap::RequestorWrap(this).receivePy();
        //        }
        //
        //        void receivePy1(bp::object& a1) {
        //            SignalSlotableWrap::RequestorWrap(this).receivePy(a1);
        //        }
        //
        //        void receivePy2(bp::object& a1, bp::object& a2) {
        //            SignalSlotableWrap::RequestorWrap(this).receivePy(a1, a2);
        //        }
        //
        //        void receivePy3(bp::object& a1, bp::object& a2, bp::object& a3) {
        //            SignalSlotableWrap::RequestorWrap(this).receivePy(a1, a2, a3);
        //        }
        //
        //        void receivePy4(bp::object& a1, bp::object& a2, bp::object& a3, bp::object& a4) {
        //            SignalSlotableWrap::RequestorWrap(this).receivePy(a1, a2, a3, a4);
        //        }

        void replyPy0() {
            registerReply(karabo::util::Hash());
        }

        void replyPy1(const bp::object& a1) {
            karabo::util::Hash reply;
            HashWrap::set(reply, "a1", a1);
            registerReply(reply);
        }

        void replyPy2(const bp::object& a1, const bp::object& a2) {
            karabo::util::Hash reply;
            HashWrap::set(reply, "a1", a1);
            HashWrap::set(reply, "a2", a2);
            registerReply(reply);
        }

        void replyPy3(const bp::object& a1, const bp::object& a2, const bp::object& a3) {
            karabo::util::Hash reply;
            HashWrap::set(reply, "a1", a1);
            HashWrap::set(reply, "a2", a2);
            HashWrap::set(reply, "a3", a3);
            registerReply(reply);
        }

        void replyPy4(const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object& a4) {
            karabo::util::Hash reply;
            HashWrap::set(reply, "a1", a1);
            HashWrap::set(reply, "a2", a2);
            HashWrap::set(reply, "a3", a3);
            HashWrap::set(reply, "a4", a4);
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

        //        void registerInstanceNewHandlerPy(const bp::object& handler) {
        //            registerInstanceNewHandler(boost::bind(&SignalSlotableWrap::proxyInstanceNewCallback,
        //                                                   this, handler, _1, _2));
        //        }

        void registerSlotCallGuardHandlerPy(const bp::object& handler) {
            registerSlotCallGuardHandler(boost::bind(&SignalSlotableWrap::proxySlotCallGuardHandler, this, handler, _1));
        }

        void registerPerformanceStatisticsHandlerPy(const bp::object& handler) {
            registerPerformanceStatisticsHandler(boost::bind(&SignalSlotableWrap::proxyUpdatePerformanceStatisticsHandler,
                                                             this, handler, _1, _2, _3));
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
            

    private:

        void proxyInstanceNotAvailableHandler(const bp::object& handler, const std::string& instanceId, const karabo::util::Hash& instanceInfo);

        void proxyInstanceAvailableAgainHandler(const bp::object& handler, const std::string& instanceId, const karabo::util::Hash& instanceInfo);

        void proxyExceptionHandler(const bp::object& handler, const karabo::util::Exception& e);

        //void proxyInstanceNewCallback(const bp::object& handler, const std::string& instanceId, const karabo::util::Hash& instanceInfo);

        bool proxySlotCallGuardHandler(const bp::object&, const std::string&);

        void proxyUpdatePerformanceStatisticsHandler(const bp::object&, float, float, unsigned int);

        void proxyOnOutputPossibleHandler(const bp::object& handler, const karabo::xms::OutputChannel::Pointer& channel);

        void proxyOnInputAvailableHandler(const bp::object& handler, const karabo::xms::InputChannel::Pointer& channel);

        void proxyOnDataAvailableHandler(const bp::object& handler, const karabo::xms::Data& data);

        void proxyOnEndOfStreamEventHandler(const bp::object& handler, const karabo::xms::InputChannel::Pointer& channel);

    private: // members
        boost::thread m_eventLoop;
    };
}

#endif  /* KARATHON_SIGNALSLOTABLE_HH */

