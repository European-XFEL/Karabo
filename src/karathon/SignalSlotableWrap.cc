/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
#include "SignalSlotableWrap.hh"

#include <karabo/util/Exception.hh>

namespace bp = boost::python;
using namespace karabo::xms;
using namespace karabo::util;
using namespace karabo::net;

namespace karathon {
    /**
     * Specialisation of HandlerWrap for error handling
     */
    class ErrorHandlerWrap : public HandlerWrap<> {
       public:
        ErrorHandlerWrap(const bp::object& handler, char const* const where) : HandlerWrap<>(handler, where) {}

        void operator()() const {
            std::string msg;
            std::string details;
            try {
                throw; // rethrow the exception
            } catch (const karabo::util::RemoteException& e) {
                details = e.type(); // contains e.g. id of remote side
                details += ":\n";
                details += e.details();
                msg = e.type();
                msg += ": ";
                msg += e.userFriendlyMsg(true);
            } catch (const karabo::util::Exception& e) {
                // No need to treat TimeoutException separately
                msg = e.userFriendlyMsg(false);
                details = e.detailedMsg();
            } catch (const std::exception& e) {
                msg = e.what();
            }
            ScopedGILAcquire gil;
            try {
                if (*m_handler) {
                    (*m_handler)(msg, details);
                }
            } catch (const bp::error_already_set& e) {
                karathon::detail::treatError_already_set(*m_handler, m_where);
            } catch (...) {
                KARABO_RETHROW
            }
        }
    };


    SignalSlotableWrap::RequestorWrap::RequestorWrap(karabo::xms::SignalSlotable* signalSlotable)
        : karabo::xms::SignalSlotable::Requestor(signalSlotable) {}

    void SignalSlotableWrap::RequestorWrap::receiveAsyncPy(const bp::object& replyCallback,
                                                           const bp::object& errorCallback, const bp::object& timeoutMs,
                                                           const bp::object& numCallbackArgs) {
        // Forward timeout if specified
        if (timeoutMs.ptr() != Py_None) {
            timeout(bp::extract<int>(timeoutMs));
        }
        // Wrap error handler
        AsyncErrorHandler errorHandler;
        if (errorCallback.ptr() != Py_None) {
            errorHandler = ErrorHandlerWrap(errorCallback, "receiveAsyncPyError");
        }
        // Figure out number of arguments of callback
        size_t numReturnArgs = 0ul;
        if (numCallbackArgs.ptr() != Py_None) { // Passed explicitely
            numReturnArgs = bp::extract<unsigned int>(numCallbackArgs);
        } else { // Deduce from callback itself
            numReturnArgs = Wrapper::numArgs(replyCallback);
        }
        switch (numReturnArgs) {
            case 0: {
                // Do everthing (incl. copying) on boost::object with GIL.
                boost::function<void()> handler(HandlerWrap<>(replyCallback, "receiveAsyncPy0"));
                // Release GIL since receiveAsync(..) in fact synchronously writes the message.
                ScopedGILRelease nogil;
                // There is no move semantics for receiveAsync (yet?), but 'handler' holds the
                // Python object for the replyCallback as a shared_ptr. So when 'handler' gets copied,
                // the Python object itself is not ==> fine to run this without GIL.
                receiveAsync(std::move(handler), std::move(errorHandler));
                break;
            }
            case 1: {
                boost::function<void(const boost::any&)> handler(HandlerWrapAny1(replyCallback, "receiveAsyncPy1"));
                ScopedGILRelease nogil;
                receiveAsync<boost::any>(std::move(handler), std::move(errorHandler));
                break;
            }
            case 2: {
                boost::function<void(const boost::any&, const boost::any&)> handler(
                      HandlerWrapAny2(replyCallback, "receiveAsyncPy2"));
                ScopedGILRelease nogil;
                receiveAsync<boost::any, boost::any>(std::move(handler), std::move(errorHandler));
                break;
            }
            case 3: {
                boost::function<void(const boost::any&, const boost::any&, const boost::any&)> handler(
                      HandlerWrapAny3(replyCallback, "receiveAsyncPy3"));
                ScopedGILRelease nogil;
                receiveAsync<boost::any, boost::any, boost::any>(std::move(handler), std::move(errorHandler));
                break;
            }
            case 4: {
                boost::function<void(const boost::any&, const boost::any&, const boost::any&, const boost::any&)>
                      handler(HandlerWrapAny4(replyCallback, "receiveAsyncPy4"));
                ScopedGILRelease nogil;
                receiveAsync<boost::any, boost::any, boost::any, boost::any>(std::move(handler),
                                                                             std::move(errorHandler));
                break;
            }
            default:
                throw KARABO_SIGNALSLOT_EXCEPTION("Detected/specified " + toString(numReturnArgs) +=
                                                  " (> 4) arguments");
        }
    }


    void SignalSlotableWrap::RequestorWrap::receiveAsyncPy0(const bp::object& replyCallback) {
        // replyCallback, errorCallback, timeoutMs, numCallbackArgs
        receiveAsyncPy(replyCallback, bp::object(), bp::object(), bp::object(0));
    }


    void SignalSlotableWrap::RequestorWrap::receiveAsyncPy1(const bp::object& replyCallback) {
        // replyCallback, errorCallback, timeoutMs, numCallbackArgs
        receiveAsyncPy(replyCallback, bp::object(), bp::object(), bp::object(1));
    }


    void SignalSlotableWrap::RequestorWrap::receiveAsyncPy2(const bp::object& replyCallback) {
        // replyCallback, errorCallback, timeoutMs, numCallbackArgs
        receiveAsyncPy(replyCallback, bp::object(), bp::object(), bp::object(2));
    }


    void SignalSlotableWrap::RequestorWrap::receiveAsyncPy3(const bp::object& replyCallback) {
        // replyCallback, errorCallback, timeoutMs, numCallbackArgs
        receiveAsyncPy(replyCallback, bp::object(), bp::object(), bp::object(3));
    }


    void SignalSlotableWrap::RequestorWrap::receiveAsyncPy4(const bp::object& replyCallback) {
        // replyCallback, errorCallback, timeoutMs, numCallbackArgs
        receiveAsyncPy(replyCallback, bp::object(), bp::object(), bp::object(4));
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
                    const boost::optional<karabo::util::Hash::Node&> detailsNode = body->find("a2");
                    const std::string& details = (detailsNode && detailsNode->is<std::string>() // since Karabo 2.14.0
                                                        ? detailsNode->getValue<std::string>()
                                                        : std::string());
                    throw karabo::util::RemoteException(text, header->get<std::string>("signalInstanceId"), details);
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
                    throw KARABO_SIGNALSLOT_EXCEPTION(
                          "Too many arguments send as response (max 4 are currently supported");
            }
        } catch (const karabo::util::RemoteException&) {
            // Just rethrow as is: No further addition to Karabo stack trace, but keeping type and details().
            throw;
        } catch (const karabo::util::TimeoutException&) {
            throw; // Rethrow as TimeoutException for proper conversion to Python TimeoutError
        } catch (const karabo::util::Exception& e) {
            // No need to add stack trace from detailesMsg() here - code converting this to Python exception will do so
            KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Error while receiving message on instance \"" +
                                                          m_signalSlotable->getInstanceId() + "\""));
            // For compiler happiness
            return bp::make_tuple();
        }
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::prepareTuple0(const karabo::util::Hash& body) {
        return bp::make_tuple();
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::prepareTuple1(const karabo::util::Hash& body) {
        bp::object a1 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a1"));
        return bp::make_tuple(a1);
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::prepareTuple2(const karabo::util::Hash& body) {
        bp::object a1 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a1"));
        bp::object a2 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a2"));
        return bp::make_tuple(a1, a2);
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::prepareTuple3(const karabo::util::Hash& body) {
        bp::object a1 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a1"));
        bp::object a2 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a2"));
        bp::object a3 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a3"));
        return bp::make_tuple(a1, a2, a3);
    }


    bp::tuple SignalSlotableWrap::RequestorWrap::prepareTuple4(const karabo::util::Hash& body) {
        bp::object a1 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a1"));
        bp::object a2 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a2"));
        bp::object a3 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a3"));
        bp::object a4 = Wrapper::deepCopyHashLike(HashWrap::get(body, "a4"));
        return bp::make_tuple(a1, a2, a3, a4);
    }


    SignalSlotableWrap::AsyncReplyWrap::AsyncReplyWrap(SignalSlotable* signalSlotable)
        : karabo::xms::SignalSlotable::AsyncReply(signalSlotable) {}


    void SignalSlotableWrap::AsyncReplyWrap::replyPy0() const {
        ScopedGILRelease nogil;
        // Call inherited operator(..) - no GIL since that synchronously sends a message:
        (*this)();
    }


    void SignalSlotableWrap::AsyncReplyWrap::replyPy1(const bp::object& a1) const {
        // Convert Python object to boost::any - may involve a copy :-(
        boost::any a1Any;
        Wrapper::toAny(a1, a1Any);
        // Call inherited operator(..) - no GIL since that synchronously sends a message:
        ScopedGILRelease nogil;
        (*this)(a1Any);
    }


    void SignalSlotableWrap::AsyncReplyWrap::replyPy2(const bp::object& a1, const bp::object& a2) const {
        // Convert Python objects to boost::any - may involve copies :-(
        boost::any a1Any, a2Any;
        Wrapper::toAny(a1, a1Any);
        Wrapper::toAny(a2, a2Any);
        // Call inherited operator(..) - no GIL since that synchronously sends a message:
        ScopedGILRelease nogil;
        (*this)(a1Any, a2Any);
    }


    void SignalSlotableWrap::AsyncReplyWrap::replyPy3(const bp::object& a1, const bp::object& a2,
                                                      const bp::object& a3) const {
        // Convert Python objects to boost::any - may involve copies :-(
        boost::any a1Any, a2Any, a3Any;
        Wrapper::toAny(a1, a1Any);
        Wrapper::toAny(a2, a2Any);
        Wrapper::toAny(a3, a3Any);
        // Call inherited operator(..) - no GIL since that synchronously sends a message:
        ScopedGILRelease nogil;
        (*this)(a1Any, a2Any, a3Any);
    }


    void SignalSlotableWrap::AsyncReplyWrap::replyPy4(const bp::object& a1, const bp::object& a2, const bp::object& a3,
                                                      const bp::object& a4) const {
        // Convert Python objects to boost::any - may involve copies :-(
        boost::any a1Any, a2Any, a3Any, a4Any;
        Wrapper::toAny(a1, a1Any);
        Wrapper::toAny(a2, a2Any);
        Wrapper::toAny(a3, a3Any);
        Wrapper::toAny(a4, a4Any);
        // Call inherited operator(..) - no GIL since that synchronously sends a message:
        ScopedGILRelease nogil;
        (*this)(a1Any, a2Any, a3Any, a4Any);
    }


    SignalSlotableWrap::SignalSlotableWrap(const std::string& instanceId,
                                           const karabo::util::Hash& connectionParameters, int heartbeatInterval,
                                           const karabo::util::Hash& instanceInfo)
        : SignalSlotable(instanceId, connectionParameters, heartbeatInterval, instanceInfo) {}


    SignalSlotableWrap::~SignalSlotableWrap() {}


    void SignalSlotableWrap::registerSlotPy(const bp::object& slotFunction, std::string slotName, int numArgs) {
        if (slotName.empty()) {
            slotName = bp::extract<std::string>((slotFunction.attr("__name__")));
        }
        boost::mutex::scoped_lock lock(m_signalSlotInstancesMutex);
        SlotInstances::const_iterator it = m_slotInstances.find(slotName);
        if (it != m_slotInstances.end()) { // Already registered
            (boost::static_pointer_cast<SlotWrap>(it->second))->registerSlotFunction(slotFunction, numArgs);
        } else {
            boost::shared_ptr<SlotWrap> s(boost::make_shared<SlotWrap>(slotName));
            s->registerSlotFunction(slotFunction, numArgs); // Bind user's slot-function to Slot
            m_slotInstances[slotName] = s;
        }
    }


    karabo::xms::InputChannel::Pointer SignalSlotableWrap::createInputChannelPy(const std::string& channelName,
                                                                                const karabo::util::Hash& config,
                                                                                const bp::object& onDataHandler,
                                                                                const bp::object& onInputHandler,
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

        return this->createInputChannel(
              channelName, config, dataHandler, inputHandler,
              HandlerWrap<const karabo::xms::InputChannel::Pointer&>(onEndOfStreamHandler, "EOS"),
              HandlerWrap<const std::string&, karabo::net::ConnectionStatus>(connectionTracker,
                                                                             "channelStatusTracker"));
    }
} // namespace karathon
