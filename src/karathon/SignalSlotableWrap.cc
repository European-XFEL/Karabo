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
        try {
            // Do everthing (incl. copying) on boost::object with GIL.
            boost::function<void ()> handler(HandlerWrap<>(replyCallback, "receiveAsyncPy0"));
            // Release GIL since receiveAsync(..) in fact synchronously writes the message.
            ScopedGILRelease nogil;
            receiveAsync(std::move(handler)); // Hopefully this 'move' avoids copy of replyCallback.
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::RequestorWrap::receiveAsyncPy1(const bp::object& replyCallback) {
        try {
            // Do everthing (incl. copying) on boost::object with GIL.
            boost::function<void (const boost::any&)> handler(HandlerWrapAny1(replyCallback, "receiveAsyncPy1"));
            // Release GIL since receiveAsync<..>(..) in fact synchronously writes the message.
            ScopedGILRelease nogil;
            receiveAsync<boost::any>(std::move(handler)); // Hopefully this 'move' avoids copy of replyCallback.
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::RequestorWrap::receiveAsyncPy2(const bp::object& replyCallback) {
        try {
            // Do everthing (incl. copying) on boost::object with GIL.
            boost::function<void (const boost::any&, const boost::any&)> handler(HandlerWrapAny2(replyCallback, "receiveAsyncPy2"));
            // Release GIL since receiveAsync<..>(..) in fact synchronously writes the message.
            ScopedGILRelease nogil;
            receiveAsync<boost::any, boost::any>(std::move(handler)); // Hopefully this 'move' avoids copy of replyCallback.
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::RequestorWrap::receiveAsyncPy3(const bp::object& replyCallback) {
        try {
            // Do everthing (incl. copying) on boost::object with GIL.
            boost::function<void (const boost::any&, const boost::any&, const boost::any&)> handler(HandlerWrapAny3(replyCallback, "receiveAsyncPy3"));
            // Release GIL since receiveAsync<..>(..) in fact synchronously writes the message.
            ScopedGILRelease nogil;
            receiveAsync<boost::any, boost::any, boost::any>(std::move(handler)); // Hopefully this 'move' avoids copy of replyCallback.
        } catch (...) {
            KARABO_RETHROW
        }
    }


    void SignalSlotableWrap::RequestorWrap::receiveAsyncPy4(const bp::object& replyCallback) {
        try {
            // Do everthing (incl. copying) on boost::object with GIL.
            boost::function<void (const boost::any&, const boost::any&, const boost::any&, const boost::any&)>
                handler(HandlerWrapAny4(replyCallback, "receiveAsyncPy4"));
            // Release GIL since receiveAsync<..>(..) in fact synchronously writes the message.
            ScopedGILRelease nogil;
            receiveAsync<boost::any, boost::any, boost::any, boost::any>(std::move(handler)); // Hopefully this 'move' avoids copy of replyCallback.
        } catch (...) {
            KARABO_RETHROW
        }
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
                    // Handling an error, so double check that input is as expected, i.e. body has key "a1":
                    const boost::optional<karabo::util::Hash::Node&> textNode = body->find("a1");
                    const std::string text(textNode && textNode->is<std::string>()
                                           ? textNode->getValue<std::string>()
                                           : "Error signaled, but body without string at key \"a1\"");
                    throw karabo::util::RemoteException(text, header->get<std::string>("signalInstanceId"));
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
        bp::object a1 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a1"));
        return bp::make_tuple(a1);
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::prepareTuple2(const karabo::util::Hash & body) {
        bp::object a1 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a1"));
        bp::object a2 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a2"));
        return bp::make_tuple(a1, a2);
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::prepareTuple3(const karabo::util::Hash & body) {
        bp::object a1 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a1"));
        bp::object a2 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a2"));
        bp::object a3 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a3"));
        return bp::make_tuple(a1, a2, a3);
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::prepareTuple4(const karabo::util::Hash & body) {
        bp::object a1 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a1"));
        bp::object a2 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a2"));
        bp::object a3 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a3"));
        bp::object a4 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a4"));
        return bp::make_tuple(a1, a2, a3, a4);
    }


    SignalSlotableWrap::SignalSlotableWrap(const std::string& instanceId,
                                           const karabo::util::Hash& connectionParameters,
                                           int heartbeatInterval,
                                           const karabo::util::Hash& instanceInfo)
        : SignalSlotable(instanceId, connectionParameters, heartbeatInterval, instanceInfo) {
    }


    SignalSlotableWrap::~SignalSlotableWrap() {
    }


    void SignalSlotableWrap::registerSlotPy(const bp::object& slotFunction, std::string slotName, int numArgs) {

        if (slotName.empty()) {
            slotName = bp::extract<std::string>((slotFunction.attr("__name__")));
        }
        boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
        SlotInstances::const_iterator it = m_slotInstances.find(slotName);
        if (it != m_slotInstances.end()) { // Already registered
            (boost::static_pointer_cast<SlotWrap >(it->second))->registerSlotFunction(slotFunction, numArgs);
        } else {
            boost::shared_ptr<SlotWrap> s(boost::make_shared<SlotWrap>(slotName));
            s->registerSlotFunction(slotFunction, numArgs); // Bind user's slot-function to Slot
            m_slotInstances[slotName] = s;
        }
    }


    karabo::xms::InputChannel::Pointer
    SignalSlotableWrap::createInputChannelPy(const std::string& channelName,
                                             const karabo::util::Hash& config,
                                             const bp::object& onDataHandler, const bp::object& onInputHandler,
                                             const bp::object& onEndOfStreamHandler,
                                             const bp::object& connectionTracker) {
        // Basically just call createInputChannel from C++, but take care that data and input handlers
        // stay empty if their input is empty, although the proxies are able
        // to deal with 'None' Python handlers (as we make use of for the end of stream handler).
        DataHandler dataHandler = DataHandler();
        InputHandler inputHandler = InputHandler();
        if (onDataHandler != bp::object()) {
            // or: if (onDataHandler.ptr() != Py_None) {
            dataHandler = InputChannelWrap::DataHandlerWrap(onDataHandler, "data");
        }
        if (onInputHandler != bp::object()) {
            inputHandler = HandlerWrap<const karabo::xms::InputChannel::Pointer&>(onInputHandler, "input");
        }

        return this->createInputChannel(channelName, config,
                                        dataHandler, inputHandler,
                                        HandlerWrap<const karabo::xms::InputChannel::Pointer&>(onEndOfStreamHandler, "EOS"),
                                        HandlerWrap<const std::string&, karabo::net::ConnectionStatus>(connectionTracker, "channelStatusTracker"));
    }
}
