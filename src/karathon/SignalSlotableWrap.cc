#include "SignalSlotableWrap.hh"

namespace bp = boost::python;
using namespace karabo::xms;
using namespace karabo::util;
using namespace karabo::net;

namespace karathon {


    SignalSlotableWrap::RequestorWrap::RequestorWrap(karabo::xms::SignalSlotable* signalSlotable)
        : karabo::xms::SignalSlotable::Requestor(signalSlotable) {
    }


    SignalSlotableWrap::RequestorWrap SignalSlotableWrap::RequestorWrap::timeoutPy(const int& milliseconds) {
        this->karabo::xms::SignalSlotable::Requestor::timeout(milliseconds);
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


    bp::tuple SignalSlotableWrap::RequestorWrap::waitForReply(const int& milliseconds) {
        try {
            timeout(milliseconds);
            karabo::util::Hash::Pointer body, header;

            {
                ScopedGILRelease nogil;
                receiveResponse(header, body);
                // The header key "error" indicates whether an exception was thrown during the remote call
                if (header->has("error") && header->get<bool>("error")) {
                    throw karabo::util::RemoteException(body->get<std::string>("a1"),
                                                        header->get<std::string>("signalInstanceId"));
                }
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
            KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Error while receiving message on instance \"" +
                                                          m_signalSlotable->getInstanceId() + "\""));
            // For compiler happiness
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
                                           int heartbeatInterval,
                                           const karabo::util::Hash& instanceInfo) : SignalSlotable() {

        JmsConnection::Pointer connection = Configurator<JmsConnection>::create(connectionType, connectionParameters);
        this->init(instanceId, connection, heartbeatInterval, instanceInfo);
    }


    SignalSlotableWrap::~SignalSlotableWrap() {
    }


    void SignalSlotableWrap::registerSlotPy(const bp::object& slotFunction) {

        std::string functionName = bp::extract<std::string>((slotFunction.attr("__name__")));
        boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
        SlotInstances::const_iterator it = m_slotInstances.find(functionName);
        if (it != m_slotInstances.end()) { // Already registered
            (boost::static_pointer_cast<SlotWrap >(it->second))->registerSlotFunction(slotFunction);
        } else {
            boost::shared_ptr<SlotWrap> s(new SlotWrap(functionName));
            s->registerSlotFunction(slotFunction); // Bind user's slot-function to Slot
            m_slotInstances[functionName] = s;
        }
    }


    karabo::xms::InputChannel::Pointer
    SignalSlotableWrap::createInputChannelPy(const std::string& channelName,
                                             const karabo::util::Hash& config,
                                             const bp::object& onDataHandler, const bp::object& onInputHandler,
                                             const bp::object& onEndOfStreamHandler) {
        // Basically just call createInputChannel from C++, but take care that
        // 'None' callbacks are really empty.
        DataHandler dataHandler = DataHandler();
        InputHandler inputHandler = InputHandler();
        InputHandler endOfStreamHandler = InputHandler();
        if (onDataHandler != bp::object()) {
            // or: if (onDataHandler.ptr() != Py_None) {
            dataHandler = boost::bind(&SignalSlotableWrap::proxyOnDataAvailableHandler,
                                      this, onDataHandler, _1);
        }
        if (onInputHandler != bp::object()) {
            inputHandler = boost::bind(&SignalSlotableWrap::proxyOnInputAvailableHandler,
                                       this, onInputHandler, _1);
        }
        if (onEndOfStreamHandler != bp::object()) {
            endOfStreamHandler = boost::bind(&SignalSlotableWrap::proxyOnEndOfStreamEventHandler,
                                             this, onEndOfStreamHandler, _1);
        }

        return this->createInputChannel(channelName, config,
                                        dataHandler, inputHandler, endOfStreamHandler);
    }


    void SignalSlotableWrap::proxyInstanceGoneHandler(const bp::object& handler,
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


    void SignalSlotableWrap::proxyInstanceNewHandler(const bp::object& handler,
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


    void SignalSlotableWrap::proxySlotCallGuardHandler(const bp::object& handler, const std::string& slotFunction) {
        ScopedGILAcquire gil;
        try {
            if (handler)
                handler(bp::object(slotFunction));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) PyErr_Print();
            throw KARABO_PYTHON_EXCEPTION("Python handler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::proxyUpdatePerformanceStatisticsHandler(const bp::object& handler,
                                                                     float avgProcessingLatency,
                                                                     unsigned int maxProcessingLatency) {
        ScopedGILAcquire gil;
        try {
            if (handler) handler(bp::object(avgProcessingLatency), bp::object(maxProcessingLatency));
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
            throw KARABO_PYTHON_EXCEPTION("Python input handler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::proxyOnDataAvailableHandler(const bp::object& handler,
                                                         const karabo::util::Hash& data) {
        ScopedGILAcquire gil;
        try {
            if (handler) handler(bp::object(data));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) PyErr_Print();
            throw KARABO_PYTHON_EXCEPTION("Python data handler has thrown an exception.");
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
            throw KARABO_PYTHON_EXCEPTION("Python EOS handler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW
        }
    }
}
