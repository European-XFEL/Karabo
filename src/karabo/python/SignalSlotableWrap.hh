/* 
 * File:   SignalSlotableWrap.hh
 * Author: <burkhard.heisen@xfel.eu>
 * Contributions: <irina.kozlova@xfel.eu>
 *
 * Created on March 19, 2012, 11:22 AM
 */

#ifndef EXFEL_PYEXFEL_SIGNALSLOTABLE_HH
#define	EXFEL_PYEXFEL_SIGNALSLOTABLE_HH

#include <boost/python.hpp>
#include <boost/function.hpp>
#include <exfel/xms/SignalSlotable.hh>

#include "SignalWrap.hh"
#include "SlotWrap.hh"
#include "MemberSlotWrap.hh"
#include "RequestorWrap.hh"

namespace bp = boost::python;

namespace exfel {

    namespace pyexfel {

        class SignalSlotableWrap : public exfel::xms::SignalSlotable {
        public:

            SignalSlotableWrap(const std::string& instanceId = "py/console/0", const std::string& connectionType = "Jms", const exfel::util::Hash& connectionParameters = exfel::util::Hash()) :
            SignalSlotable() {
                if (!PyEval_ThreadsInitialized())
                        PyEval_InitThreads();
                exfel::net::BrokerConnection::Pointer connection = exfel::net::BrokerConnection::create(connectionType, connectionParameters);
                this->init(connection, instanceId);
                m_eventLoop = boost::thread(boost::bind(&exfel::xms::SignalSlotable::runEventLoop, this, true));
            }

            virtual ~SignalSlotableWrap() {
                this->stopEventLoop();
                m_eventLoop.join();
            }
            
            bp::list getAvailableInstancesPy() {
                return HashWrap::stdVector2pyList(this->getAvailableInstances());
            }
            
            bp::list getAvailableSignalsPy(const std::string& instanceId) {
                return HashWrap::stdVector2pyList(this->getAvailableSignals(instanceId));
            }
            
            bp::list getAvailableSlotsPy(const std::string& instanceId) {
                return HashWrap::stdVector2pyList(this->getAvailableSlots(instanceId));
            }

            void registerSlotPy(const bp::object& slotFunction, const SlotType& slotType = SPECIFIC) {
                std::string functionName = bp::extract<std::string>((slotFunction.attr("func_name")));
                if (m_slotInstances.find(functionName) != m_slotInstances.end()) return; // Already registered
                exfel::net::BrokerChannel::Pointer channel = m_connection->createChannel(); // New Channel
                std::string instanceId = prepareInstanceId(slotType);
                boost::shared_ptr<SlotWrap> s(new SlotWrap(this, channel, instanceId, functionName)); // New specific slot
                s->registerSlotFunction(slotFunction.ptr()); // Bind user's slot-function to Slot
                storeSlot(functionName, s, channel); // Keep slot and his channel alive
            }
            
            void registerMemberSlotPy(const bp::object& slotFunction, const bp::object& selfObject, const SlotType& slotType = SPECIFIC) {
                std::string functionName = bp::extract<std::string>((slotFunction.attr("func_name")));
                if (m_slotInstances.find(functionName) != m_slotInstances.end()) return; // Already registered
                exfel::net::BrokerChannel::Pointer channel = m_connection->createChannel(); // New Channel
                std::string instanceId = prepareInstanceId(slotType);
                boost::shared_ptr<MemberSlotWrap> s(new MemberSlotWrap(this, channel, instanceId, functionName)); // New specific slot
                s->registerSlotFunction(functionName, selfObject.ptr()); // Bind user's slot-function to Slot
                storeSlot(functionName, s, channel); // Keep slot and his channel alive
            }
            
            void registerSignalPy0(const std::string& funcName) {
                this->registerSignal(funcName);
            }

            void registerSignalPy1(const std::string& funcName, const bp::object&) {
                boost::shared_ptr<exfel::pyexfel::SignalWrap> s(new exfel::pyexfel::SignalWrap(m_signalChannel, m_instanceId, funcName));
                boost::function<void (const bp::object&) > f(boost::bind(&exfel::pyexfel::SignalWrap::emitPy1, s, _1));
                storeSignal(funcName, s, f);
            }

            void registerSignalPy2(const std::string& funcName, const bp::object&, const bp::object&) {
                boost::shared_ptr<exfel::pyexfel::SignalWrap> s(new exfel::pyexfel::SignalWrap(m_signalChannel, m_instanceId, funcName));
                boost::function<void (const bp::object&, const bp::object&) > f(boost::bind(&exfel::pyexfel::SignalWrap::emitPy2, s, _1, _2));
                storeSignal(funcName, s, f);
            }

            void registerSignalPy3(const std::string& funcName, const bp::object&, const bp::object&, const bp::object&) {
                boost::shared_ptr<exfel::pyexfel::SignalWrap> s(new exfel::pyexfel::SignalWrap(m_signalChannel, m_instanceId, funcName));
                boost::function<void (const bp::object&, const bp::object&, const bp::object&) > f(boost::bind(&exfel::pyexfel::SignalWrap::emitPy3, s, _1, _2, _3));
                storeSignal(funcName, s, f);
            }

            void registerSignalPy4(const std::string& funcName, const bp::object&, const bp::object&, const bp::object&, const bp::object&) {
                boost::shared_ptr<exfel::pyexfel::SignalWrap> s(new exfel::pyexfel::SignalWrap(m_signalChannel, m_instanceId, funcName));
                boost::function<void (const bp::object&, const bp::object&, const bp::object&, const bp::object&) > f(boost::bind(&exfel::pyexfel::SignalWrap::emitPy4, s, _1, _2, _3, _4));
                storeSignal(funcName, s, f);
            }

            void callPy1(std::string instanceId, const std::string& functionName, const bp::object& a1) const {
                SignalWrap s(m_signalChannel, m_instanceId, "call");
                if (instanceId.empty()) instanceId = m_instanceId;
                s.registerSlot(instanceId, functionName);
                s.emitPy1(a1);
            }

            void callPy2(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2) const {
                SignalWrap s(m_signalChannel, m_instanceId, "call");
                if (instanceId.empty()) instanceId = m_instanceId;
                s.registerSlot(instanceId, functionName);
                s.emitPy2(a1, a2);
            }

            void callPy3(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2, const bp::object& a3) const {
                SignalWrap s(m_signalChannel, m_instanceId, "call");
                if (instanceId.empty()) instanceId = m_instanceId;
                s.registerSlot(instanceId, functionName);
                s.emitPy3(a1, a2, a3);
            }

            void callPy4(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object& a4) const {
                SignalWrap s(m_signalChannel, m_instanceId, "call");
                if (instanceId.empty()) instanceId = m_instanceId;
                s.registerSlot(instanceId, functionName);
                s.emitPy4(a1, a2, a3, a4);
            }
            
            void reconfigurePy(const std::string& instanceId, const std::string& key, const bp::object& value) const {
                exfel::util::Hash configuration;      
                exfel::pyexfel::HashWrap::pythonSet(configuration, key, value);
                reconfigurePy(instanceId, configuration);
            }
            
            void reconfigurePy(const std::string& instanceId, const exfel::util::Hash& configuration) const {
                call(instanceId, "slotReconfigure", configuration);
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
                if (!m_isProcessingSlot) return;
                registerReply(exfel::util::Hash());
                
            }
            
            void replyPy1(const bp::object& a1) {
                if (!m_isProcessingSlot) return;
                exfel::util::Hash reply;
                HashWrap::pythonSet(reply, "a1", a1);
                registerReply(reply);
            }
            
            void replyPy2(const bp::object& a1, const bp::object& a2) {
                if (!m_isProcessingSlot) return;
                exfel::util::Hash reply;
                HashWrap::pythonSet(reply, "a1", a1);
                HashWrap::pythonSet(reply, "a2", a2);
                registerReply(reply);
                
            }
            
            void replyPy3(const bp::object& a1, const bp::object& a2, const bp::object& a3) {
                if(!m_isProcessingSlot) return;
                exfel::util::Hash reply;
                HashWrap::pythonSet(reply, "a1", a1);
                HashWrap::pythonSet(reply, "a2", a2);
                HashWrap::pythonSet(reply, "a3", a3);
                registerReply(reply);
            }
            
            void replyPy4(const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object& a4) {
                if(!m_isProcessingSlot) return;
                exfel::util::Hash reply;
                HashWrap::pythonSet(reply, "a1", a1);
                HashWrap::pythonSet(reply, "a2", a2);
                HashWrap::pythonSet(reply, "a3", a3);
                HashWrap::pythonSet(reply, "a4", a4);
                registerReply(reply);
            }
            

        private: // members
            boost::thread m_eventLoop;

        };
    }
}

#endif	/* EXFEL_PYEXFEL_SIGNALSLOTABLE_HH */

