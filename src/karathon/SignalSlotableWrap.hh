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
#include <karabo/xms/SignalSlotable.hh>
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

            explicit RequestorWrap(karabo::xms::SignalSlotable* signalSlotable)
            : karabo::xms::SignalSlotable::Requestor(signalSlotable) {
            }

            RequestorWrap timeoutPy(const int& milliseconds) {
                this->karabo::xms::SignalSlotable::Requestor::timeout(milliseconds);
                return *this;
            }

            RequestorWrap& requestPy(const std::string& slotInstanceId,
                                     const std::string& slotFunction) {
                try {
                    karabo::util::Hash payload;
                    ScopedGILRelease nogil;
                    sendRequest(prepareHeader(slotInstanceId, slotFunction), payload);
                } catch (...) {
                    KARABO_RETHROW
                }
                return *this;
            }

            RequestorWrap& requestPy(const std::string& slotInstanceId,
                                     const std::string& slotFunction,
                                     const bp::object& a1) {
                try {
                    karabo::util::Hash payload;
                    HashWrap::set(payload, "a1", a1);

                    ScopedGILRelease nogil;
                    sendRequest(prepareHeader(slotInstanceId, slotFunction), payload);
                } catch (...) {
                    KARABO_RETHROW
                }
                return *this;
            }

            RequestorWrap& requestPy(const std::string& slotInstanceId,
                                     const std::string& slotFunction,
                                     const bp::object& a1,
                                     const bp::object& a2) {
                try {
                    karabo::util::Hash payload;
                    HashWrap::set(payload, "a1", a1);
                    HashWrap::set(payload, "a2", a2);

                    ScopedGILRelease nogil;
                    sendRequest(prepareHeader(slotInstanceId, slotFunction), payload);
                } catch (...) {
                    KARABO_RETHROW
                }
                return *this;
            }

            RequestorWrap& requestPy(const std::string& slotInstanceId,
                                     const std::string& slotFunction,
                                     const bp::object& a1,
                                     const bp::object& a2,
                                     const bp::object& a3) {
                try {
                    karabo::util::Hash payload;
                    HashWrap::set(payload, "a1", a1);
                    HashWrap::set(payload, "a2", a2);
                    HashWrap::set(payload, "a3", a3);

                    ScopedGILRelease nogil;
                    sendRequest(prepareHeader(slotInstanceId, slotFunction), payload);
                } catch (...) {
                    KARABO_RETHROW
                }
                return *this;
            }

            RequestorWrap& requestPy(const std::string& slotInstanceId,
                                     const std::string& slotFunction,
                                     const bp::object& a1,
                                     const bp::object& a2,
                                     const bp::object& a3,
                                     const bp::object& a4) {
                try {
                    karabo::util::Hash payload;
                    HashWrap::set(payload, "a1", a1);
                    HashWrap::set(payload, "a2", a2);
                    HashWrap::set(payload, "a3", a3);
                    HashWrap::set(payload, "a4", a4);

                    ScopedGILRelease nogil;
                    sendRequest(prepareHeader(slotInstanceId, slotFunction), payload);
                } catch (...) {
                    KARABO_RETHROW
                }
                return *this;
            }

            RequestorWrap& requestNoWaitPy(const std::string& requestSlotInstanceId,
                                           const std::string& requestSlotFunction,
                                           const std::string replySlotInstanceId,
                                           const std::string& replySlotFunction) {
                try {
                    karabo::util::Hash payload;
                    ScopedGILRelease nogil;
                    sendRequest(prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction), payload);
                } catch (...) {
                    KARABO_RETHROW
                }
                return *this;
            }

            RequestorWrap& requestNoWaitPy(const std::string& requestSlotInstanceId,
                                           const std::string& requestSlotFunction,
                                           const std::string replySlotInstanceId,
                                           const std::string& replySlotFunction,
                                           const bp::object& a1) {
                try {
                    karabo::util::Hash payload;
                    HashWrap::set(payload, "a1", a1);

                    ScopedGILRelease nogil;
                    sendRequest(prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction), payload);
                } catch (...) {
                    KARABO_RETHROW
                }
                return *this;
            }

            RequestorWrap & requestNoWaitPy(const std::string& requestSlotInstanceId,
                                            const std::string& requestSlotFunction,
                                            const std::string replySlotInstanceId,
                                            const std::string& replySlotFunction,
                                            const bp::object& a1, const bp::object & a2) {
                try {
                    karabo::util::Hash payload;
                    HashWrap::set(payload, "a1", a1);
                    HashWrap::set(payload, "a2", a2);

                    ScopedGILRelease nogil;
                    sendRequest(prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction), payload);
                } catch (...) {
                    KARABO_RETHROW
                }
                return *this;
            }

            RequestorWrap & requestNoWaitPy(const std::string& requestSlotInstanceId,
                                            const std::string& requestSlotFunction,
                                            const std::string replySlotInstanceId,
                                            const std::string& replySlotFunction, const bp::object& a1, const bp::object& a2, const bp::object & a3) {
                try {
                    karabo::util::Hash payload;
                    HashWrap::set(payload, "a1", a1);
                    HashWrap::set(payload, "a2", a2);
                    HashWrap::set(payload, "a3", a3);

                    ScopedGILRelease nogil;
                    sendRequest(prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction), payload);
                } catch (...) {
                    KARABO_RETHROW
                }
                return *this;
            }

            RequestorWrap & requestNoWaitPy(const std::string& requestSlotInstanceId,
                                            const std::string& requestSlotFunction,
                                            const std::string replySlotInstanceId,
                                            const std::string& replySlotFunction, const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object & a4) {
                try {
                    karabo::util::Hash payload;
                    HashWrap::set(payload, "a1", a1);
                    HashWrap::set(payload, "a2", a2);
                    HashWrap::set(payload, "a3", a3);
                    HashWrap::set(payload, "a4", a4);

                    ScopedGILRelease nogil;
                    sendRequest(prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction), payload);
                } catch (...) {
                    KARABO_RETHROW
                }
                return *this;
            }

            void receiveAsyncPy0(const bp::object& replyCallback) {
                if (!PyCallable_Check(replyCallback.ptr()))
                    throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
                try {
                    ScopedGILRelease nogil;
                    receiveAsync<bp::object>(boost::bind(&SignalSlotableWrap::RequestorWrap::proxyReceiveAsync0, this, replyCallback));
                } catch (...) {
                    KARABO_RETHROW
                }
            }

            void proxyReceiveAsync0(const bp::object& replyCallback) {
                ScopedGILAcquire gil;
                replyCallback();
            }

            void receiveAsyncPy1(const bp::object& replyCallback) {
                if (!PyCallable_Check(replyCallback.ptr()))
                    throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
                try {
                    ScopedGILRelease nogil;
                    receiveAsync<boost::any>(boost::bind(&SignalSlotableWrap::RequestorWrap::proxyReceiveAsync1, this, replyCallback, _1));
                } catch (...) {
                    KARABO_RETHROW
                }
            }

            void proxyReceiveAsync1(const bp::object& replyCallback, const boost::any& a1) {
                ScopedGILAcquire gil;
                replyCallback(Wrapper::toObject(a1));
            }

            void receiveAsyncPy2(const bp::object& replyCallback) {
                if (!PyCallable_Check(replyCallback.ptr()))
                    throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
                try {
                    ScopedGILRelease nogil;
                    receiveAsync<boost::any, boost::any>(boost::bind(&SignalSlotableWrap::RequestorWrap::proxyReceiveAsync2, this, replyCallback, _1, _2));
                } catch (...) {
                    KARABO_RETHROW
                }
            }

            void proxyReceiveAsync2(const bp::object& replyCallback, const boost::any& a1, const boost::any& a2) {
                ScopedGILAcquire gil;
                replyCallback( Wrapper::toObject(a1), Wrapper::toObject(a2));
            }

            void receiveAsyncPy3(const bp::object& replyCallback) {
                if (!PyCallable_Check(replyCallback.ptr()))
                    throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
                try {
                    ScopedGILRelease nogil;
                    receiveAsync<boost::any, boost::any, boost::any>(boost::bind(&SignalSlotableWrap::RequestorWrap::proxyReceiveAsync3, this, replyCallback, _1, _2, _3));
                } catch (...) {
                    KARABO_RETHROW
                }
            }

            void proxyReceiveAsync3(const bp::object& replyCallback, const boost::any& a1, const boost::any& a2, const boost::any& a3) {
                ScopedGILAcquire gil;
                replyCallback(Wrapper::toObject(a1), Wrapper::toObject(a2), Wrapper::toObject(a3));
            }

            void receiveAsyncPy4(const bp::object& replyCallback) {
                if (!PyCallable_Check(replyCallback.ptr()))
                    throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
                try {
                    ScopedGILRelease nogil;
                    receiveAsync<boost::any, boost::any, boost::any, boost::any>(boost::bind(&SignalSlotableWrap::RequestorWrap::proxyReceiveAsync4, this, replyCallback, _1, _2, _3, _4));
                } catch (...) {
                    KARABO_RETHROW
                }
            }

            void proxyReceiveAsync4(const bp::object& replyCallback, const boost::any& a1, const boost::any& a2, const boost::any& a3, const boost::any& a4) {
                ScopedGILAcquire gil;
                replyCallback(Wrapper::toObject(a1), Wrapper::toObject(a2), Wrapper::toObject(a3), Wrapper::toObject(a4));
            }

            bp::tuple receivePy0() {
                ScopedGILRelease nogil;
                receive();
                return bp::make_tuple();
            }

            bp::tuple receivePy1() {
                boost::any a1;

                {
                    ScopedGILRelease nogil;
                    receive(a1);
                }

                return bp::make_tuple(Wrapper::toObject(a1));
            }

            bp::tuple receivePy2() {
                boost::any a1;
                boost::any a2;

                {
                    ScopedGILRelease nogil;
                    receive(a1, a2);
                }

                return bp::make_tuple(Wrapper::toObject(a1), Wrapper::toObject(a2));
            }

            bp::tuple receivePy3() {
                boost::any a1;
                boost::any a2;
                boost::any a3;

                {
                    ScopedGILRelease nogil;
                    receive(a1, a2, a3);
                }

                return bp::make_tuple(Wrapper::toObject(a1), Wrapper::toObject(a2), Wrapper::toObject(a3));
            }

            bp::tuple receivePy4() {
                boost::any a1;
                boost::any a2;
                boost::any a3;
                boost::any a4;

                {
                    ScopedGILRelease nogil;
                    receive(a1, a2, a3, a4);
                }

                return bp::make_tuple(Wrapper::toObject(a1), Wrapper::toObject(a2), Wrapper::toObject(a3), Wrapper::toObject(a4));
            }

            bp::tuple waitForReply(const int& milliseconds) {
                try {
                    timeout(milliseconds);
                    karabo::util::Hash::Pointer body, header;

                    {
                        ScopedGILRelease nogil;
                        receiveResponse(header, body);
                    }

                    size_t arity = 0; // body.size()
                    for (karabo::util::Hash::const_iterator it = body->begin(); it != body->end(); it++) {
                        std::string key = it->getKey();
                        if (key == "a1" || key == "a2" || key == "a3" || key == "a4") arity++;
                    }

                    switch (arity) {
                        case 0:
                            return prepareTuple0(*body);
                        case 1:
                            return prepareTuple1(*body);
                        case 2:
                            return prepareTuple2(*body);
                        case 3:
                            return prepareTuple3(*body);
                        case 4:
                            return prepareTuple4(*body);
                        default:
                            throw KARABO_SIGNALSLOT_EXCEPTION("Too many arguments send as response (max 4 are currently supported");
                    }
                } catch (const karabo::util::Exception& e) {
                    std::cout << e << std::endl;
                    return bp::make_tuple();
                }
            }

            bp::tuple prepareTuple0(const karabo::util::Hash & body) {
                return bp::make_tuple();
            }

            bp::tuple prepareTuple1(const karabo::util::Hash & body) {
                bp::object a1 = HashWrap::get(body, "a1");
                return bp::make_tuple(a1);
            }

            bp::tuple prepareTuple2(const karabo::util::Hash & body) {
                bp::object a1 = HashWrap::get(body, "a1");
                bp::object a2 = HashWrap::get(body, "a2");
                return bp::make_tuple(a1, a2);
            }

            bp::tuple prepareTuple3(const karabo::util::Hash & body) {
                bp::object a1 = HashWrap::get(body, "a1");
                bp::object a2 = HashWrap::get(body, "a2");
                bp::object a3 = HashWrap::get(body, "a3");
                return bp::make_tuple(a1, a2, a3);
            }

            bp::tuple prepareTuple4(const karabo::util::Hash & body) {
                bp::object a1 = HashWrap::get(body, "a1");
                bp::object a2 = HashWrap::get(body, "a2");
                bp::object a3 = HashWrap::get(body, "a3");
                bp::object a4 = HashWrap::get(body, "a4");
                return bp::make_tuple(a1, a2, a3, a4);
            }
        };

    public:

        SignalSlotableWrap(const std::string& instanceId = generateInstanceId<SignalSlotable>(),
                           const std::string& connectionType = "Jms",
                           const karabo::util::Hash& connectionParameters = karabo::util::Hash(),
                           const bool autostartEventLoop = true,
                           int heartbeatInterval = 10) : SignalSlotable() {

            karabo::net::BrokerConnection::Pointer connection = karabo::net::BrokerConnection::create(connectionType, connectionParameters);
            this->init(instanceId, connection);

            if (autostartEventLoop) {
                ScopedGILRelease nogil;
                m_eventLoop = boost::thread(boost::bind(&karabo::xms::SignalSlotable::runEventLoop, this, heartbeatInterval, karabo::util::Hash(), 2));
                boost::this_thread::sleep(boost::posix_time::milliseconds(10)); // give a chance above thread to start up before leaving this constructor
            }
        }

        virtual ~SignalSlotableWrap() {
            this->stopEventLoop();
            if (m_eventLoop.joinable())
                m_eventLoop.join();
        }

        static boost::shared_ptr<SignalSlotableWrap> create(const std::string& instanceId = generateInstanceId<SignalSlotable>(),
                                                            const std::string& connectionType = "Jms",
                                                            const karabo::util::Hash& connectionParameters = karabo::util::Hash(),
                                                            bool autostart = false,
                                                            int heartbeatInterval = 10) {
            return boost::shared_ptr<SignalSlotableWrap>(new SignalSlotableWrap(instanceId, connectionType, connectionParameters, autostart, heartbeatInterval));
        }

        void runEventLoop(int emitHeartbeat = 10, const karabo::util::Hash& info = karabo::util::Hash(), int nThreads = 2) {
            ScopedGILRelease nogil;
            karabo::xms::SignalSlotable::runEventLoop(emitHeartbeat, info, nThreads);
        }

        void stopEventLoop() {
            ScopedGILRelease nogil;
            karabo::xms::SignalSlotable::stopEventLoop();
        }

        bp::object getInstanceId() {
            return bp::object(SignalSlotable::getInstanceId());
        }

        bp::tuple exists(const std::string& instanceId) {
            std::pair<bool, std::string> result;
            {
                ScopedGILRelease nogil;
                result = SignalSlotable::exists(instanceId);
            }
            return bp::make_tuple(result.first, bp::object(result.second));
        }

        bp::object getAvailableInstancesPy(bool activateTracking) {
            return Wrapper::fromStdVectorToPyList(this->getAvailableInstances(activateTracking));
        }

        bp::object getAvailableSignalsPy(const std::string& instanceId) {
            return Wrapper::fromStdVectorToPyList(this->getAvailableSignals(instanceId));
        }

        bp::object getAvailableSlotsPy(const std::string& instanceId) {
            return Wrapper::fromStdVectorToPyList(this->getAvailableSlots(instanceId));
        }

        void registerSlotPy(const bp::object& slotFunction, const SlotType& slotType = LOCAL) {

            std::string functionName = bp::extract<std::string>((slotFunction.attr("__name__")));
            SlotInstances* slotInstances;
            if (slotType == LOCAL) slotInstances = &(m_localSlotInstances);
            else slotInstances = &(m_globalSlotInstances);


            SlotInstances::const_iterator it = slotInstances->find(functionName);
            if (it != slotInstances->end()) { // Already registered
                (boost::static_pointer_cast<SlotWrap >(it->second))->registerSlotFunction(slotFunction);
            } else {
                boost::shared_ptr<SlotWrap> s(new SlotWrap(functionName));
                s->registerSlotFunction(slotFunction); // Bind user's slot-function to Slot
                (*slotInstances)[functionName] = s;
            }
        }

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

        //        template <class T>
        //        boost::shared_ptr<karabo::io::Input<T> > registerInputChannel(const std::string& name,
        //                const std::string& type,
        //                const karabo::util::Hash& config,
        //                const bp::object& onRead,
        //                const bp::object& onEndOfStream) {
        //
        //            karabo::io::AbstractInput::Pointer channel = karabo::io::Input<T>::create(type, config);
        //            channel->setInstanceId(m_instanceId);
        //            channel->setInputHandlerType("python", std::string(typeid (typename karabo::io::Input<T>).name()));
        //            if (onRead != bp::object()) {
        //                channel->registerIOEventHandler(onRead);
        //            }
        //            if (onEndOfStream != bp::object()) {
        //                channel->registerEndOfStreamEventHandler(onEndOfStream);
        //            }
        //            m_inputChannels[name] = channel;
        //            return boost::static_pointer_cast<karabo::io::Input<T> >(channel);
        //
        //        }
        //
        //        template <class T>
        //        boost::shared_ptr<karabo::io::Output<T> > registerOutputChannel(const std::string& name,
        //                const std::string& type,
        //                const karabo::util::Hash& config,
        //                const bp::object& onOutputPossibleHandler) {
        //            using namespace karabo::util;
        //
        //            karabo::io::AbstractOutput::Pointer channel = karabo::io::Output<T>::create(type, config);
        //
        //            channel->setInstanceId(m_instanceId);
        //            channel->setOutputHandlerType("python");
        //            if (onOutputPossibleHandler != bp::object()) {
        //                channel->registerIOEventHandler(onOutputPossibleHandler);
        //            }
        //            m_outputChannels[name] = channel;
        //            return boost::static_pointer_cast<karabo::io::Output<T> >(channel);
        //        }
        //
        //        template <class InputType>
        //        boost::shared_ptr<InputType > createInputChannel(const std::string& name, const karabo::util::Hash& input,
        //                const bp::object& onInputAvailableHandler, const bp::object& onEndOfStreamEventHandler) {
        //            using namespace karabo::util;
        //
        //            karabo::io::AbstractInput::Pointer channel = InputType::createChoice(name, input);
        //
        //            channel->setInstanceId(m_instanceId);
        //            channel->setInputHandlerType("python", std::string(typeid (InputType).name()));
        //            if (onInputAvailableHandler != bp::object()) {
        //                channel->registerIOEventHandler(onInputAvailableHandler);
        //            }
        //            if (onEndOfStreamEventHandler != bp::object()) {
        //                channel->registerEndOfStreamEventHandler(onEndOfStreamEventHandler);
        //            }
        //            m_inputChannels[name] = channel;
        //            return boost::static_pointer_cast<InputType >(channel);
        //        }
        //
        //        template <class OutputType>
        //        boost::shared_ptr<OutputType > createOutputChannel(const std::string& name, const karabo::util::Hash& input,
        //                const bp::object& onOutputPossibleHandler) {
        //            using namespace karabo::util;
        //
        //            karabo::io::AbstractOutput::Pointer channel = OutputType::createChoice(name, input);
        //
        //            channel->setInstanceId(m_instanceId);
        //            channel->setOutputHandlerType("python");
        //            if (onOutputPossibleHandler != bp::object()) {
        //                channel->registerIOEventHandler(onOutputPossibleHandler);
        //            }
        //            m_outputChannels[name] = channel;
        //            return boost::static_pointer_cast<OutputType>(channel);
        //        }
        //
        //        bp::object getInputChannels() {
        //            typedef std::map<std::string, karabo::io::AbstractInput::Pointer> InputChannels;
        //            const InputChannels& ichannels = karabo::xms::SignalSlotable::getInputChannels();
        //            bp::dict d;
        //            for (InputChannels::const_iterator it = ichannels.begin(); it != ichannels.end(); ++it) {
        //                d[it->first] = bp::object(it->second);
        //            }
        //            return bp::object(d);
        //        }
        //
        //        bp::object getOutputChannels() {
        //            typedef std::map<std::string, karabo::io::AbstractOutput::Pointer> OutputChannels;
        //            const OutputChannels& ochannels = karabo::xms::SignalSlotable::getOutputChannels();
        //            bp::dict d;
        //            for (OutputChannels::const_iterator it = ochannels.begin(); it != ochannels.end(); ++it) {
        //                d[it->first] = bp::object(it->second);
        //            }
        //            return bp::object(d);
        //        }

        bp::object getBrokerHost() {
            return bp::object(getConnection()->getBrokerHostname() + ":" + karabo::util::toString(getConnection()->getBrokerPort()));
        }

        bp::object getBrokerTopic() {
            return bp::object(getConnection()->getBrokerTopic());
        }

        bp::object getBrokerHosts() {
            return Wrapper::fromStdVectorToPyList<std::string>(getConnection()->getBrokerHosts());
        }

    private: // members
        boost::thread m_eventLoop;
    };
}

#endif  /* KARATHON_SIGNALSLOTABLE_HH */

