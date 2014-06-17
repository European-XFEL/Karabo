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

#include "SignalWrap.hh"
#include "SlotWrap.hh"
#include "MemberSlotWrap.hh"
#include "RequestorWrap.hh"
#include "ScopedGILRelease.hh"

namespace bp = boost::python;

namespace karathon {

    class SignalSlotableWrap : public karabo::xms::SignalSlotable {

    public:

        SignalSlotableWrap(const std::string& instanceId = generateInstanceId<SignalSlotable>(),
                           const std::string& connectionType = "Jms",
                           const karabo::util::Hash& connectionParameters = karabo::util::Hash(),
                           const bool autostartEventLoop = true,
                           int heartbeatInterval = 10) : SignalSlotable() {

            if (!PyEval_ThreadsInitialized()) PyEval_InitThreads();

            karabo::net::BrokerConnection::Pointer connection = karabo::net::BrokerConnection::create(connectionType, connectionParameters);
            this->init(instanceId, connection);

            if (autostartEventLoop) {
                ScopedGILRelease nogil;
                m_eventLoop = boost::thread(boost::bind(&karabo::xms::SignalSlotable::runEventLoop, this, heartbeatInterval, karabo::util::Hash()));
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

        void runEventLoop(int emitHeartbeat = 10, const karabo::util::Hash& info = karabo::util::Hash()) {
            ScopedGILRelease nogil;
            karabo::xms::SignalSlotable::runEventLoop(emitHeartbeat, info);
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

        void registerSignalPy0(const std::string& funcName) {
            this->registerSignal(funcName);
        }

        void registerSignalPy1(const std::string& funcName, const bp::object&) {
            boost::shared_ptr<karathon::SignalWrap> s(new karathon::SignalWrap(this, m_signalChannel, m_instanceId, funcName));
            boost::function<void (const bp::object&) > f(boost::bind(&karathon::SignalWrap::emitPy1, s, _1));
            storeSignal(funcName, s, f);
        }

        void registerSignalPy2(const std::string& funcName, const bp::object&, const bp::object&) {
            boost::shared_ptr<karathon::SignalWrap> s(new karathon::SignalWrap(this, m_signalChannel, m_instanceId, funcName));
            boost::function<void (const bp::object&, const bp::object&) > f(boost::bind(&karathon::SignalWrap::emitPy2, s, _1, _2));
            storeSignal(funcName, s, f);
        }

        void registerSignalPy3(const std::string& funcName, const bp::object&, const bp::object&, const bp::object&) {
            boost::shared_ptr<karathon::SignalWrap> s(new karathon::SignalWrap(this, m_signalChannel, m_instanceId, funcName));
            boost::function<void (const bp::object&, const bp::object&, const bp::object&) > f(boost::bind(&karathon::SignalWrap::emitPy3, s, _1, _2, _3));
            storeSignal(funcName, s, f);
        }

        void registerSignalPy4(const std::string& funcName, const bp::object&, const bp::object&, const bp::object&, const bp::object&) {
            boost::shared_ptr<karathon::SignalWrap> s(new karathon::SignalWrap(this, m_signalChannel, m_instanceId, funcName));
            boost::function<void (const bp::object&, const bp::object&, const bp::object&, const bp::object&) > f(boost::bind(&karathon::SignalWrap::emitPy4, s, _1, _2, _3, _4));
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

        template <class T>
        boost::shared_ptr<karabo::io::Input<T> > registerInputChannel(const std::string& name,
                                                                      const std::string& type,
                                                                      const karabo::util::Hash& config,
                                                                      const bp::object& onRead,
                                                                      const bp::object& onEndOfStream) {

            karabo::io::AbstractInput::Pointer channel = karabo::io::Input<T>::create(type, config);
            channel->setInstanceId(m_instanceId);
            channel->setInputHandlerType("python", std::string(typeid (typename karabo::io::Input<T>).name()));
            if (onRead != bp::object()) {
                channel->registerIOEventHandler(onRead);
            }
            if (onEndOfStream != bp::object()) {
                channel->registerEndOfStreamEventHandler(onEndOfStream);
            }
            m_inputChannels[name] = channel;
            return boost::static_pointer_cast<karabo::io::Input<T> >(channel);

        }

        template <class T>
        boost::shared_ptr<karabo::io::Output<T> > registerOutputChannel(const std::string& name,
                                                                        const std::string& type,
                                                                        const karabo::util::Hash& config,
                                                                        const bp::object& onOutputPossibleHandler) {
            using namespace karabo::util;

            karabo::io::AbstractOutput::Pointer channel = karabo::io::Output<T>::create(type, config);

            channel->setInstanceId(m_instanceId);
            channel->setOutputHandlerType("python");
            if (onOutputPossibleHandler != bp::object()) {
                channel->registerIOEventHandler(onOutputPossibleHandler);
            }
            m_outputChannels[name] = channel;
            return boost::static_pointer_cast<karabo::io::Output<T> >(channel);
        }

        template <class InputType>
        boost::shared_ptr<InputType > createInputChannel(const std::string& name, const karabo::util::Hash& input,
                                                         const bp::object& onInputAvailableHandler, const bp::object& onEndOfStreamEventHandler) {
            using namespace karabo::util;

            karabo::io::AbstractInput::Pointer channel = InputType::createChoice(name, input);

            channel->setInstanceId(m_instanceId);
            channel->setInputHandlerType("python", std::string(typeid (InputType).name()));
            if (onInputAvailableHandler != bp::object()) {
                channel->registerIOEventHandler(onInputAvailableHandler);
            }
            if (onEndOfStreamEventHandler != bp::object()) {
                channel->registerEndOfStreamEventHandler(onEndOfStreamEventHandler);
            }
            m_inputChannels[name] = channel;
            return boost::static_pointer_cast<InputType >(channel);
        }

        template <class OutputType>
        boost::shared_ptr<OutputType > createOutputChannel(const std::string& name, const karabo::util::Hash& input,
                                                           const bp::object& onOutputPossibleHandler) {
            using namespace karabo::util;

            karabo::io::AbstractOutput::Pointer channel = OutputType::createChoice(name, input);

            channel->setInstanceId(m_instanceId);
            channel->setOutputHandlerType("python");
            if (onOutputPossibleHandler != bp::object()) {
                channel->registerIOEventHandler(onOutputPossibleHandler);
            }
            m_outputChannels[name] = channel;
            return boost::static_pointer_cast<OutputType>(channel);
        }

        bp::object getInputChannels() {
            typedef std::map<std::string, karabo::io::AbstractInput::Pointer> InputChannels;
            InputChannels ichannels = karabo::xms::SignalSlotable::getInputChannels();
            bp::dict d;
            for (InputChannels::const_iterator it = ichannels.begin(); it != ichannels.end(); ++it) {
                
            }
            return bp::object(karabo::xms::SignalSlotable::getInputChannels());
        }

        bp::object getOutputChannels() {
            return bp::object(karabo::xms::SignalSlotable::getOutputChannels());
        }

    private: // members
        boost::thread m_eventLoop;

    };
}

#endif  /* KARATHON_SIGNALSLOTABLE_HH */

