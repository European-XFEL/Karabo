/* 
 * File:   SignalSlotableWrap.hh
 * Author: <burkhard.heisen@xfel.eu>
 * Contributions: <irina.kozlova@xfel.eu>
 *
 * Created on March 19, 2012, 11:22 AM
 */

#ifndef KARABO_PYKARABO_SIGNALSLOTABLE_HH
#define	KARABO_PYKARABO_SIGNALSLOTABLE_HH

#include <boost/python.hpp>
#include <boost/function.hpp>
#include <karabo/xms/SignalSlotable.hh>

#include "SignalWrap.hh"
#include "SlotWrap.hh"
#include "MemberSlotWrap.hh"
#include "RequestorWrap.hh"
#include "ScopedGILRelease.hh"

namespace bp = boost::python;

namespace karabo {

    namespace pyexfel {

        class SignalSlotableWrap : public karabo::xms::SignalSlotable {

        public:

            SignalSlotableWrap(const std::string& instanceId = "py/console/0",
                               const std::string& connectionType = "Jms",
                               const karabo::util::Hash& connectionParameters = karabo::util::Hash(),
                               bool autostart = true) : SignalSlotable() {
                if (!PyEval_ThreadsInitialized())
                    PyEval_InitThreads();
                karabo::net::BrokerConnection::Pointer connection = karabo::net::BrokerConnection::create(connectionType, connectionParameters);
                this->init(connection, instanceId);
                if (autostart)
                    m_eventLoop = boost::thread(boost::bind(&karabo::xms::SignalSlotable::runEventLoop, this, true, karabo::util::Hash())); // TODO put instance info here
            }

            virtual ~SignalSlotableWrap() {
                this->stopEventLoop();
                if (m_eventLoop.joinable())
                    m_eventLoop.join();
            }

            static boost::shared_ptr<SignalSlotableWrap> create(const std::string& instanceId = "py/console/0",
                                                                const std::string& connectionType = "Jms",
                                                                const karabo::util::Hash& connectionParameters = karabo::util::Hash(),
                                                                bool autostart = false) {
                return boost::shared_ptr<SignalSlotableWrap>(new SignalSlotableWrap(instanceId, connectionType, connectionParameters, autostart));
            }

            void runEventLoop(bool emitHeartbeat = true, const karabo::util::Hash& info = karabo::util::Hash()) {
                ScopedGILRelease nogil;
                karabo::xms::SignalSlotable::runEventLoop(emitHeartbeat, info);
            }

            bp::object getAvailableInstancesPy() {
                return Wrapper::fromStdVectorToPyList(this->getAvailableInstances());
            }

            bp::object getAvailableSignalsPy(const std::string& instanceId) {
                return Wrapper::fromStdVectorToPyList(this->getAvailableSignals(instanceId));
            }

            bp::object getAvailableSlotsPy(const std::string& instanceId) {
                return Wrapper::fromStdVectorToPyList(this->getAvailableSlots(instanceId));
            }

            void registerSlotPy(const bp::object& slotFunction, const SlotType& slotType = SPECIFIC) {
                std::string functionName = bp::extract<std::string>((slotFunction.attr("func_name")));
                if (m_slotInstances.find(functionName) != m_slotInstances.end()) return; // Already registered
                karabo::net::BrokerChannel::Pointer channel = m_connection->createChannel(); // New Channel
                std::string instanceId = prepareInstanceId(slotType);
                if (Wrapper::hasattr(slotFunction, "__self__")) { // Member function
                    const bp::object & selfObject(slotFunction.attr("__self__"));
                    std::string funcName(bp::extract<std::string > (slotFunction.attr("__name__")));
                    boost::shared_ptr<MemberSlotWrap> s(new MemberSlotWrap(this, channel, instanceId, functionName)); // New specific slot
                    s->registerSlotFunction(funcName, selfObject.ptr()); // Bind user's slot-function to Slot
                    storeSlot(functionName, s, channel); // Keep slot and his channel alive
                } else { // Free function
                    boost::shared_ptr<SlotWrap> s(new SlotWrap(this, channel, instanceId, functionName)); // New specific slot
                    s->registerSlotFunction(slotFunction.ptr()); // Bind user's slot-function to Slot
                    storeSlot(functionName, s, channel); // Keep slot and his channel alive
                }
            }

            //            void registerMemberSlotPy(const bp::object& slotFunction, const bp::object& selfObject, const SlotType& slotType = SPECIFIC) {
            //                std::string functionName = bp::extract<std::string>((slotFunction.attr("func_name")));
            //                if (m_slotInstances.find(functionName) != m_slotInstances.end()) return; // Already registered
            //                karabo::net::BrokerChannel::Pointer channel = m_connection->createChannel(); // New Channel
            //                std::string instanceId = prepareInstanceId(slotType);
            //                boost::shared_ptr<MemberSlotWrap> s(new MemberSlotWrap(this, channel, instanceId, functionName)); // New specific slot
            //                s->registerSlotFunction(functionName, selfObject.ptr()); // Bind user's slot-function to Slot
            //                storeSlot(functionName, s, channel); // Keep slot and his channel alive
            //            }

            void registerSignalPy0(const std::string& funcName) {
                this->registerSignal(funcName);
            }

            void registerSignalPy1(const std::string& funcName, const bp::object&) {
                boost::shared_ptr<karabo::pyexfel::SignalWrap> s(new karabo::pyexfel::SignalWrap(this, m_signalChannel, m_instanceId, funcName));
                boost::function<void (const bp::object&) > f(boost::bind(&karabo::pyexfel::SignalWrap::emitPy1, s, _1));
                storeSignal(funcName, s, f);
            }

            void registerSignalPy2(const std::string& funcName, const bp::object&, const bp::object&) {
                boost::shared_ptr<karabo::pyexfel::SignalWrap> s(new karabo::pyexfel::SignalWrap(this, m_signalChannel, m_instanceId, funcName));
                boost::function<void (const bp::object&, const bp::object&) > f(boost::bind(&karabo::pyexfel::SignalWrap::emitPy2, s, _1, _2));
                storeSignal(funcName, s, f);
            }

            void registerSignalPy3(const std::string& funcName, const bp::object&, const bp::object&, const bp::object&) {
                boost::shared_ptr<karabo::pyexfel::SignalWrap> s(new karabo::pyexfel::SignalWrap(this, m_signalChannel, m_instanceId, funcName));
                boost::function<void (const bp::object&, const bp::object&, const bp::object&) > f(boost::bind(&karabo::pyexfel::SignalWrap::emitPy3, s, _1, _2, _3));
                storeSignal(funcName, s, f);
            }

            void registerSignalPy4(const std::string& funcName, const bp::object&, const bp::object&, const bp::object&, const bp::object&) {
                boost::shared_ptr<karabo::pyexfel::SignalWrap> s(new karabo::pyexfel::SignalWrap(this, m_signalChannel, m_instanceId, funcName));
                boost::function<void (const bp::object&, const bp::object&, const bp::object&, const bp::object&) > f(boost::bind(&karabo::pyexfel::SignalWrap::emitPy4, s, _1, _2, _3, _4));
                storeSignal(funcName, s, f);
            }

            void callPy1(std::string instanceId, const std::string& functionName, const bp::object& a1) const {
                SignalWrap s(this, m_signalChannel, m_instanceId, "call");
                if (instanceId.empty()) instanceId = m_instanceId;
                s.registerSlot(instanceId, functionName);
                s.emitPy1(a1);
            }

            void callPy2(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2) const {
                SignalWrap s(this, m_signalChannel, m_instanceId, "call");
                if (instanceId.empty()) instanceId = m_instanceId;
                s.registerSlot(instanceId, functionName);
                s.emitPy2(a1, a2);
            }

            void callPy3(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2, const bp::object& a3) const {
                SignalWrap s(this, m_signalChannel, m_instanceId, "call");
                if (instanceId.empty()) instanceId = m_instanceId;
                s.registerSlot(instanceId, functionName);
                s.emitPy3(a1, a2, a3);
            }

            void callPy4(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object& a4) const {
                SignalWrap s(this, m_signalChannel, m_instanceId, "call");
                if (instanceId.empty()) instanceId = m_instanceId;
                s.registerSlot(instanceId, functionName);
                s.emitPy4(a1, a2, a3, a4);
            }

            void emitPy1(const std::string& signalFunction, const bp::object& a1) {
                this->emit(signalFunction, a1);
            }

            void emitPy2(const std::string& signalFunction, const bp::object& a1, const bp::object& a2) {
                this->emit(signalFunction, a1, a2);
            }

            void emitPy3(const std::string& signalFunction, const bp::object& a1, const bp::object& a2, const bp::object& a3) {
                this->emit(signalFunction, a1, a2, a3);
            }

            void emitPy4(const std::string& signalFunction, const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object& a4) {
                this->emit(signalFunction, a1, a2, a3, a4);
            }

            RequestorWrap requestPy0(std::string instanceId, const std::string& functionName) {
                if (instanceId.empty()) instanceId = m_instanceId;
                return RequestorWrap(m_requestChannel, m_instanceId).callPy(instanceId, functionName);
            }

            RequestorWrap requestPy1(std::string instanceId, const std::string& functionName, const bp::object& a1) {
                if (instanceId.empty()) instanceId = m_instanceId;
                return RequestorWrap(m_requestChannel, m_instanceId).callPy(instanceId, functionName, a1);
            }

            RequestorWrap requestPy2(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2) {
                if (instanceId.empty()) instanceId = m_instanceId;
                return RequestorWrap(m_requestChannel, m_instanceId).callPy(instanceId, functionName, a1, a2);
            }

            RequestorWrap requestPy3(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2, const bp::object& a3) {
                if (instanceId.empty()) instanceId = m_instanceId;
                return RequestorWrap(m_requestChannel, m_instanceId).callPy(instanceId, functionName, a1, a2, a3);
            }

            RequestorWrap requestPy4(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object& a4) {
                if (instanceId.empty()) instanceId = m_instanceId;
                return RequestorWrap(m_requestChannel, m_instanceId).callPy(instanceId, functionName, a1, a2, a3, a4);
            }

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


        private: // members
            boost::thread m_eventLoop;

        };
    }
}

#endif

