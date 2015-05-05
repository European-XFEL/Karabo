#include "SignalSlotableWrap.hh"

namespace bp = boost::python;
using namespace karabo::xms;

namespace karathon {


    SignalSlotableWrap::RequestorWrap::RequestorWrap(karabo::xms::SignalSlotable* signalSlotable)
    : karabo::xms::SignalSlotable::Requestor(signalSlotable) {
    }


    SignalSlotableWrap::RequestorWrap SignalSlotableWrap::RequestorWrap::timeoutPy(const int& milliseconds) {
        this->karabo::xms::SignalSlotable::Requestor::timeout(milliseconds);
        return *this;
    }


    SignalSlotableWrap::RequestorWrap& SignalSlotableWrap::RequestorWrap::requestPy(const std::string& slotInstanceId,
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


    SignalSlotableWrap::RequestorWrap& SignalSlotableWrap::RequestorWrap::requestPy(const std::string& slotInstanceId,
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


    SignalSlotableWrap::RequestorWrap& SignalSlotableWrap::RequestorWrap::requestPy(const std::string& slotInstanceId,
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


    SignalSlotableWrap::RequestorWrap& SignalSlotableWrap::RequestorWrap::requestPy(const std::string& slotInstanceId,
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


    SignalSlotableWrap::RequestorWrap& SignalSlotableWrap::RequestorWrap::requestPy(const std::string& slotInstanceId,
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


    SignalSlotableWrap::RequestorWrap& SignalSlotableWrap::RequestorWrap::requestNoWaitPy(const std::string& requestSlotInstanceId,
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


    SignalSlotableWrap::RequestorWrap& SignalSlotableWrap::RequestorWrap::requestNoWaitPy(const std::string& requestSlotInstanceId,
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


    SignalSlotableWrap::RequestorWrap & SignalSlotableWrap::RequestorWrap::requestNoWaitPy(const std::string& requestSlotInstanceId,
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


    SignalSlotableWrap::RequestorWrap & SignalSlotableWrap::RequestorWrap::requestNoWaitPy(const std::string& requestSlotInstanceId,
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


    SignalSlotableWrap::RequestorWrap & SignalSlotableWrap::RequestorWrap::requestNoWaitPy(const std::string& requestSlotInstanceId,
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


    void SignalSlotableWrap::RequestorWrap::receiveAsyncPy0(const bp::object& replyCallback) {
        if (!PyCallable_Check(replyCallback.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        try {
            ScopedGILRelease nogil;
            receiveAsync<bp::object>(boost::bind(&SignalSlotableWrap::RequestorWrap::proxyReceiveAsync0, this, replyCallback));
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::RequestorWrap::proxyReceiveAsync0(const bp::object& replyCallback) {
        ScopedGILAcquire gil;
        replyCallback();
    }


    void SignalSlotableWrap::RequestorWrap::receiveAsyncPy1(const bp::object& replyCallback) {
        if (!PyCallable_Check(replyCallback.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        try {
            ScopedGILRelease nogil;
            receiveAsync<boost::any>(boost::bind(&SignalSlotableWrap::RequestorWrap::proxyReceiveAsync1, this, replyCallback, _1));
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::RequestorWrap::proxyReceiveAsync1(const bp::object& replyCallback, const boost::any& a1) {
        ScopedGILAcquire gil;
        replyCallback(Wrapper::toObject(a1));
    }


    void SignalSlotableWrap::RequestorWrap::receiveAsyncPy2(const bp::object& replyCallback) {
        if (!PyCallable_Check(replyCallback.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        try {
            ScopedGILRelease nogil;
            receiveAsync<boost::any, boost::any>(boost::bind(&SignalSlotableWrap::RequestorWrap::proxyReceiveAsync2, this, replyCallback, _1, _2));
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::RequestorWrap::proxyReceiveAsync2(const bp::object& replyCallback, const boost::any& a1, const boost::any& a2) {
        ScopedGILAcquire gil;
        replyCallback(Wrapper::toObject(a1), Wrapper::toObject(a2));
    }


    void SignalSlotableWrap::RequestorWrap::receiveAsyncPy3(const bp::object& replyCallback) {
        if (!PyCallable_Check(replyCallback.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        try {
            ScopedGILRelease nogil;
            receiveAsync<boost::any, boost::any, boost::any>(boost::bind(&SignalSlotableWrap::RequestorWrap::proxyReceiveAsync3, this, replyCallback, _1, _2, _3));
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::RequestorWrap::proxyReceiveAsync3(const bp::object& replyCallback, const boost::any& a1, const boost::any& a2, const boost::any& a3) {
        ScopedGILAcquire gil;
        replyCallback(Wrapper::toObject(a1), Wrapper::toObject(a2), Wrapper::toObject(a3));
    }


    void SignalSlotableWrap::RequestorWrap::receiveAsyncPy4(const bp::object& replyCallback) {
        if (!PyCallable_Check(replyCallback.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        try {
            ScopedGILRelease nogil;
            receiveAsync<boost::any, boost::any, boost::any, boost::any>(boost::bind(&SignalSlotableWrap::RequestorWrap::proxyReceiveAsync4, this, replyCallback, _1, _2, _3, _4));
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::RequestorWrap::proxyReceiveAsync4(const bp::object& replyCallback, const boost::any& a1, const boost::any& a2, const boost::any& a3, const boost::any& a4) {
        ScopedGILAcquire gil;
        replyCallback(Wrapper::toObject(a1), Wrapper::toObject(a2), Wrapper::toObject(a3), Wrapper::toObject(a4));
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::receivePy0() {
        ScopedGILRelease nogil;
        receive();
        return bp::make_tuple();
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::receivePy1() {
        boost::any a1;

        {
            ScopedGILRelease nogil;
            receive(a1);
        }

        return bp::make_tuple(Wrapper::toObject(a1));
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::receivePy2() {
        boost::any a1;
        boost::any a2;

        {
            ScopedGILRelease nogil;
            receive(a1, a2);
        }

        return bp::make_tuple(Wrapper::toObject(a1), Wrapper::toObject(a2));
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::receivePy3() {
        boost::any a1;
        boost::any a2;
        boost::any a3;

        {
            ScopedGILRelease nogil;
            receive(a1, a2, a3);
        }

        return bp::make_tuple(Wrapper::toObject(a1), Wrapper::toObject(a2), Wrapper::toObject(a3));
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::receivePy4() {
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


    bp::tuple SignalSlotableWrap::RequestorWrap::waitForReply(const int& milliseconds) {
        try {
            timeout(milliseconds);
            karabo::util::Hash::Pointer body, header;

            {
                ScopedGILRelease nogil;
                receiveResponse(header, body);
            }

            size_t arity = body->size();

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


    bp::tuple SignalSlotableWrap::RequestorWrap::prepareTuple0(const karabo::util::Hash & body) {
        return bp::make_tuple();
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::prepareTuple1(const karabo::util::Hash & body) {
        bp::object a1 = HashWrap::get(body, "a1");
        return bp::make_tuple(a1);
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::prepareTuple2(const karabo::util::Hash & body) {
        bp::object a1 = HashWrap::get(body, "a1");
        bp::object a2 = HashWrap::get(body, "a2");
        return bp::make_tuple(a1, a2);
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::prepareTuple3(const karabo::util::Hash & body) {
        bp::object a1 = HashWrap::get(body, "a1");
        bp::object a2 = HashWrap::get(body, "a2");
        bp::object a3 = HashWrap::get(body, "a3");
        return bp::make_tuple(a1, a2, a3);
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::prepareTuple4(const karabo::util::Hash & body) {
        bp::object a1 = HashWrap::get(body, "a1");
        bp::object a2 = HashWrap::get(body, "a2");
        bp::object a3 = HashWrap::get(body, "a3");
        bp::object a4 = HashWrap::get(body, "a4");
        return bp::make_tuple(a1, a2, a3, a4);
    }


    SignalSlotableWrap::SignalSlotableWrap(const std::string& instanceId,
                                           const std::string& connectionType,
                                           const karabo::util::Hash& connectionParameters,
                                           const bool autostartEventLoop,
                                           int heartbeatInterval) : SignalSlotable() {

        karabo::net::BrokerConnection::Pointer connection = karabo::net::BrokerConnection::create(connectionType, connectionParameters);
        this->init(instanceId, connection);

        if (autostartEventLoop) {
            ScopedGILRelease nogil;
            m_eventLoop = boost::thread(boost::bind(&karabo::xms::SignalSlotable::runEventLoop, this, heartbeatInterval, karabo::util::Hash(), 2));
            boost::this_thread::sleep(boost::posix_time::milliseconds(10)); // give a chance above thread to start up before leaving this constructor
        }
    }


    SignalSlotableWrap::~SignalSlotableWrap() {
        this->stopEventLoop();
        if (m_eventLoop.joinable())
            m_eventLoop.join();
    }


    void SignalSlotableWrap::registerSlotPy(const bp::object& slotFunction, const SlotType& slotType) {

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


    void SignalSlotableWrap::proxyInstanceNotAvailableHandler(const bp::object& handler,
                                                              const std::string& instanceId,
                                                              const karabo::util::Hash& instanceInfo) {
        ScopedGILAcquire gil;
        try {
            if (handler) handler(bp::object(instanceId), bp::object(instanceInfo));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) PyErr_Print();
            throw KARABO_PYTHON_EXCEPTION("Python handler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::proxyInstanceAvailableAgainHandler(const bp::object& handler,
                                                                const std::string& instanceId,
                                                                const karabo::util::Hash& instanceInfo) {
        ScopedGILAcquire gil;
        try {
            if (handler) handler(bp::object(instanceId), bp::object(instanceInfo));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) PyErr_Print();
            throw KARABO_PYTHON_EXCEPTION("Python handler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::proxyExceptionHandler(const bp::object& handler,
                                                   const karabo::util::Exception& e) {
        ScopedGILAcquire gil;
        try {
            if (handler) handler(bp::object(e.userFriendlyMsg()), bp::object(e.detailedMsg()));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) PyErr_Print();
            throw KARABO_PYTHON_EXCEPTION("Python handler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW
        }
    }


    //void SignalSlotableWrap::proxyInstanceNewCallback(const bp::object& handler,
    //                                                  const std::string& instanceId,
    //                                                  const karabo::util::Hash& instanceInfo) {
    //    ScopedGILAcquire gil;
    //    try {
    //        if (handler) handler(bp::object(instanceId), bp::object(instanceInfo));
    //    } catch (const bp::error_already_set& e) {
    //        if (PyErr_Occurred()) PyErr_Print();
    //        throw KARABO_PYTHON_EXCEPTION("Python handler has thrown an exception.");
    //    } catch (...) {
    //        KARABO_RETHROW
    //    }
    //}


    bool SignalSlotableWrap::proxySlotCallGuardHandler(const bp::object& handler,
                                                       const std::string& slotFunction,
                                                       std::string& errorMessage) {
        ScopedGILAcquire gil;
        try {
            if (handler) return handler(bp::object(slotFunction), bp::object(errorMessage));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) PyErr_Print();
            throw KARABO_PYTHON_EXCEPTION("Python handler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW
        }
        return true;
    }


    void SignalSlotableWrap::proxyUpdatePerformanceStatisticsHandler(const bp::object& handler,
                                                                     float brokerLatency,
                                                                     float processingLatency,
                                                                     unsigned int queueSize) {
        ScopedGILAcquire gil;
        try {
            if (handler) handler(bp::object(brokerLatency), bp::object(processingLatency), bp::object(queueSize));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) PyErr_Print();
            throw KARABO_PYTHON_EXCEPTION("Python handler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::proxyOnOutputPossibleHandler(const bp::object& handler,
                                                          const karabo::xms::OutputChannel::Pointer& channel) {
        ScopedGILAcquire gil;
        try {
            if (handler) handler(bp::object(channel));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) PyErr_Print();
            throw KARABO_PYTHON_EXCEPTION("Python handler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::proxyOnInputAvailableHandler(const bp::object& handler,
                                                          const karabo::xms::InputChannel::Pointer& channel) {
        ScopedGILAcquire gil;
        try {
            if (handler) handler(bp::object(channel));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) PyErr_Print();
            throw KARABO_PYTHON_EXCEPTION("Python handler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::proxyOnEndOfStreamEventHandler(const bp::object& handler,
                                                            const karabo::xms::InputChannel::Pointer& channel) {
        ScopedGILAcquire gil;
        try {
            if (handler) handler(bp::object(channel));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) PyErr_Print();
            throw KARABO_PYTHON_EXCEPTION("Python handler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW
        }
    }
}
