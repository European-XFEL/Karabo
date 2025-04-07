/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <pybind11/pybind11.h>

#include <karabo/xms/InputChannel.hh>
#include <karabo/xms/SignalSlotable.hh>
#include <karabo/xms/Slot.hh>

#include "HandlerWrap.hh"
#include "Wrapper.hh"

namespace py = pybind11;


namespace karabind {

    class ErrorHandlerWrap : public HandlerWrap<> {
       public:
        ErrorHandlerWrap(const py::object& handler, const char* const where) : HandlerWrap<>(handler, where) {}
        void operator()() const {
            std::string msg;
            std::string details;
            try {
                throw; // rethrow the exception
            } catch (const karabo::data::RemoteException& e) {
                details = e.type(); // contains e.g. id of remote side
                details += ":\n";
                details += e.details();
                msg = e.type();
                msg += ": ";
                msg += e.userFriendlyMsg(true);
            } catch (const karabo::data::Exception& e) {
                // No need to treat TimeoutException separately
                msg = e.userFriendlyMsg(false);
                details = e.detailedMsg();
            } catch (const std::exception& e) {
                msg = e.what();
            }
            py::gil_scoped_acquire gil;
            try {
                if (*m_handler) {
                    (*m_handler)(msg, details);
                }
            } catch (py::error_already_set& e) {
                karabind::detail::treatError_already_set(e, *m_handler, m_where);
            } catch (...) {
                KARABO_RETHROW
            }
        }
    };


    // Construct 'SignalSlotableWrap' class, 'SlotWrap' class and corresponding 'RequestorWrap' subclass to help
    // in binding since C++ 'karabo::xms::SignalSlotable::Requestor' is "protected" and C++ 'karabo::xms::Slot"
    // has protected destructor.  Solution is to create "XXXWrap" classes to fix such problems and encapsulate
    // some binding logic as well.  Try to use copy from karathon ...

    class SlotWrap : public karabo::xms::Slot {
        // unique_ptr seems more light weight than shared_ptr (no thread guarantees)
        std::unique_ptr<py::object> m_slotFunction;
        size_t m_arity; // arity of position arguments, except *args

       public:
        SlotWrap(const std::string& slotFunction) : karabo::xms::Slot(slotFunction) {}

        virtual ~SlotWrap() {
            py::gil_scoped_acquire gil; // for py::object destructor
            m_slotFunction.reset();
        }

        void registerSlotFunction(const py::object& slotHandler, int numArgs) {
            // We accept ONLY the python callable
            if (!PyCallable_Check(slotHandler.ptr())) {
                throw KARABO_PARAMETER_EXCEPTION("The argument is not callable.");
            }

            // Note: In C++ there is a list of slot handlers to which is appended - here we overwrite any previous
            // handler. Note 2: Create pointer to be able to ensure that deletion can be done with GIL acquired.
            std::unique_ptr<py::object> newHandler(std::make_unique<py::object>(slotHandler));
            m_slotFunction.swap(newHandler);
            if (numArgs >= 0) {
                m_arity = numArgs;
            } else {
                // Undefined number of arguments, try to figure out
                m_arity = wrapper::numArgs(*m_slotFunction);
            }
        }

       private: // function
        void doCallRegisteredSlotFunctions(const karabo::data::Hash& body) {
            py::gil_scoped_acquire gil;
            switch (m_arity) {
                case 4:
                    callFunction4(body);
                    break;
                case 3:
                    callFunction3(body);
                    break;
                case 2:
                    callFunction2(body);
                    break;
                case 1:
                    callFunction1(body);
                    break;
                case 0:
                    callFunction0(body);
                    break;
                default:
                    throw KARABO_SIGNALSLOT_EXCEPTION(
                          "Too many arguments send to python slot (max 4 are currently supported");
            }
        }

        void callFunction0(const karabo::data::Hash& body) {
            try {
                (*m_slotFunction)();
            } catch (py::error_already_set& e) {
                rethrowPythonException(e);
            }
        }

        void callFunction1(const karabo::data::Hash& body) {
            py::object a1 = getBodyArgument(body, "a1");
            try {
                (*m_slotFunction)(a1);
            } catch (py::error_already_set& e) {
                rethrowPythonException(e);
            }
        }

        void callFunction2(const karabo::data::Hash& body) {
            py::object a1 = getBodyArgument(body, "a1");
            py::object a2 = getBodyArgument(body, "a2");
            try {
                (*m_slotFunction)(a1, a2);
            } catch (py::error_already_set& e) {
                rethrowPythonException(e);
            }
        }

        void callFunction3(const karabo::data::Hash& body) {
            py::object a1 = getBodyArgument(body, "a1");
            py::object a2 = getBodyArgument(body, "a2");
            py::object a3 = getBodyArgument(body, "a3");
            try {
                (*m_slotFunction)(a1, a2, a3);
            } catch (py::error_already_set& e) {
                rethrowPythonException(e);
            }
        }

        void callFunction4(const karabo::data::Hash& body) {
            py::object a1 = getBodyArgument(body, "a1");
            py::object a2 = getBodyArgument(body, "a2");
            py::object a3 = getBodyArgument(body, "a3");
            py::object a4 = getBodyArgument(body, "a4");
            try {
                (*m_slotFunction)(a1, a2, a3, a4);
            } catch (py::error_already_set& e) {
                rethrowPythonException(e);
            }
        }

        /// Helper for callFunction<N> to get argument with given key ("a1", ..., or "a4") out of body.
        /// Throws if that key is missing.
        py::object getBodyArgument(const karabo::data::Hash& body, const char* key) const {
            // Avoid using HashWrap::get: it returns None if key missing (but we want the exception!).
            // Since get uses internally this getRef with the same const_cast, that is safe here as well
            return karabind::hashwrap::getRef(const_cast<karabo::data::Hash&>(body), key, ".");
        }

        void rethrowPythonException(py::error_already_set& e) {
            std::string pythonErrorMessage, pythonErrorDetails;
            std::tie(pythonErrorMessage, pythonErrorDetails) = getPythonExceptionStrings(e);
            throw KARABO_PYTHON_EXCEPTION2(pythonErrorMessage, pythonErrorDetails);
        }
    };


    class SignalSlotableWrap : public karabo::xms::SignalSlotable {
       public:
        KARABO_CLASSINFO(SignalSlotableWrap, "SignalSlotableWrap", "1.0")

        class RequestorWrap : public karabo::xms::SignalSlotable::Requestor {
           public:
            explicit RequestorWrap(karabo::xms::SignalSlotable* signalSlotable)
                : karabo::xms::SignalSlotable::Requestor(signalSlotable) {}

            RequestorWrap timeoutPy(const int& milliseconds);

            template <typename... Args>
            RequestorWrap& requestPy(const std::string& slotInstanceId, const std::string& slotFunction,
                                     const Args&... args) {
                try {
                    auto body = std::make_shared<karabo::data::Hash>();
                    packPy(*body, args...);
                    py::gil_scoped_release release;
                    registerRequest(slotInstanceId, prepareRequestHeader(slotInstanceId, slotFunction), body);
                } catch (...) {
                    KARABO_RETHROW
                }
                return *this;
            }

            template <typename... Args>
            RequestorWrap& requestNoWaitPy(const std::string& requestSlotInstanceId,
                                           const std::string& requestSlotFunction,
                                           const std::string replySlotInstanceId, const std::string& replySlotFunction,
                                           const Args&... args) {
                try {
                    auto body = std::make_shared<karabo::data::Hash>();
                    packPy(*body, args...);
                    py::gil_scoped_release release;
                    karabo::data::Hash::Pointer header = prepareRequestNoWaitHeader(
                          requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction);
                    registerRequest(requestSlotInstanceId, header, body);
                    sendRequest();
                } catch (...) {
                    KARABO_RETHROW
                }
                return *this;
            }

            void receiveAsyncPy(const py::object& replyCallback, const py::object& errorCallback = py::none(),
                                const py::object& timeoutMs = py::none(),
                                const py::object& numCallbackArgs = py::none()) {
                // Forward timeout if specified
                if (timeoutMs != py::none()) {
                    timeout(timeoutMs.cast<int>());
                }
                // Wrap error handler
                AsyncErrorHandler errorHandler;
                if (errorCallback != py::none()) {
                    errorHandler = ErrorHandlerWrap(errorCallback, "receiveAsyncPyError");
                }
                // Figure out number of arguments of callback
                size_t numReturnArgs = 0ul;
                if (numCallbackArgs != py::none()) { // Passed explicitely
                    numReturnArgs = numCallbackArgs.cast<unsigned int>();
                } else { // Deduce from callback itself
                    numReturnArgs = wrapper::numArgs(replyCallback);
                }
                switch (numReturnArgs) {
                    case 0: {
                        // Do everthing (incl. copying) on std::object with GIL.
                        std::function<void()> handler(HandlerWrap<>(replyCallback, "receiveAsyncPy0"));
                        // Release GIL since receiveAsync(..) in fact synchronously writes the message.
                        py::gil_scoped_release release;
                        // There is no move semantics for receiveAsync (yet?), but 'handler' holds the
                        // Python object for the replyCallback as a shared_ptr. So when 'handler' gets copied,
                        // the Python object itself is not ==> fine to run this without GIL.
                        receiveAsync(std::move(handler), std::move(errorHandler));
                        break;
                    }
                    case 1: {
                        std::function<void(const std::any&)> handler(HandlerWrapAny1(replyCallback, "receiveAsyncPy1"));
                        py::gil_scoped_release release;
                        receiveAsync<std::any>(std::move(handler), std::move(errorHandler));
                        break;
                    }
                    case 2: {
                        std::function<void(const std::any&, const std::any&)> handler(
                              HandlerWrapAny2(replyCallback, "receiveAsyncPy2"));
                        py::gil_scoped_release release;
                        receiveAsync<std::any, std::any>(std::move(handler), std::move(errorHandler));
                        break;
                    }
                    case 3: {
                        std::function<void(const std::any&, const std::any&, const std::any&)> handler(
                              HandlerWrapAny3(replyCallback, "receiveAsyncPy3"));
                        py::gil_scoped_release release;
                        receiveAsync<std::any, std::any, std::any>(std::move(handler), std::move(errorHandler));
                        break;
                    }
                    case 4: {
                        std::function<void(const std::any&, const std::any&, const std::any&, const std::any&)> handler(
                              HandlerWrapAny4(replyCallback, "receiveAsyncPy4"));
                        py::gil_scoped_release release;
                        receiveAsync<std::any, std::any, std::any, std::any>(std::move(handler),
                                                                             std::move(errorHandler));
                        break;
                    }
                    default:
                        throw KARABO_SIGNALSLOT_EXCEPTION("Detected/specified " + std::to_string(numReturnArgs) +=
                                                          " (> 4) arguments");
                }
            }

            void receiveAsyncPy0(const py::object& replyCallback) {
                // replyCallback, errorCallback, timeoutMs, numCallbackArgs
                receiveAsyncPy(replyCallback, py::none(), py::none(), py::cast(0));
            }

            void receiveAsyncPy1(const py::object& replyCallback) {
                // replyCallback, errorCallback, timeoutMs, numCallbackArgs
                receiveAsyncPy(replyCallback, py::none(), py::none(), py::cast(1));
            }

            void receiveAsyncPy2(const py::object& replyCallback) {
                // replyCallback, errorCallback, timeoutMs, numCallbackArgs
                receiveAsyncPy(replyCallback, py::none(), py::none(), py::cast(2));
            }

            void receiveAsyncPy3(const py::object& replyCallback) {
                // replyCallback, errorCallback, timeoutMs, numCallbackArgs
                receiveAsyncPy(replyCallback, py::none(), py::none(), py::cast(3));
            }

            void receiveAsyncPy4(const py::object& replyCallback) {
                // replyCallback, errorCallback, timeoutMs, numCallbackArgs
                receiveAsyncPy(replyCallback, py::none(), py::none(), py::cast(4));
            }

            py::tuple waitForReply(const int& milliseconds) {
                try {
                    timeout(milliseconds);
                    karabo::data::Hash::Pointer body, header;

                    {
                        py::gil_scoped_release release;
                        receiveResponse(header, body);
                        // The header key "error" indicates whether an exception was thrown during the remote call
                        if (header->has("error") && header->get<bool>("error")) {
                            // Handling an error, so double check that input is as expected, i.e. body has key "a1":
                            const boost::optional<karabo::data::Hash::Node&> textNode = body->find("a1");
                            const std::string text(textNode && textNode->is<std::string>()
                                                         ? textNode->getValue<std::string>()
                                                         : "Error signaled, but body without string at key \"a1\"");
                            const boost::optional<karabo::data::Hash::Node&> detailsNode = body->find("a2");
                            const std::string& details =
                                  (detailsNode && detailsNode->is<std::string>() ? detailsNode->getValue<std::string>()
                                                                                 : std::string());
                            throw karabo::data::RemoteException(text, header->get<std::string>("signalInstanceId"),
                                                                details);
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
                } catch (const karabo::data::RemoteException&) {
                    // Just rethrow as is: No further addition to Karabo stack trace, but keeping type and details().
                    throw;
                } catch (const karabo::data::TimeoutException&) {
                    throw; // Rethrow as TimeoutException for proper conversion to Python TimeoutError
                } catch (const karabo::data::Exception& e) {
                    // No need to add stack trace from detailesMsg() here - code converting this to Python exception
                    // will do so
                    KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Error while receiving message on instance \"" +
                                                                  m_signalSlotable->getInstanceId() + "\""));
                    // For compiler happiness
                    return py::make_tuple();
                }
            }

           private:
            py::tuple prepareTuple0(const karabo::data::Hash& body) {
                return py::make_tuple();
            }

            py::tuple prepareTuple1(const karabo::data::Hash& body) {
                py::object a1 = wrapper::deepCopyHashLike(hashwrap::get(body, "a1"));
                return py::make_tuple(a1);
            }

            py::tuple prepareTuple2(const karabo::data::Hash& body) {
                py::object a1 = wrapper::deepCopyHashLike(hashwrap::get(body, "a1"));
                py::object a2 = wrapper::deepCopyHashLike(hashwrap::get(body, "a2"));
                return py::make_tuple(a1, a2);
            }

            py::tuple prepareTuple3(const karabo::data::Hash& body) {
                py::object a1 = wrapper::deepCopyHashLike(hashwrap::get(body, "a1"));
                py::object a2 = wrapper::deepCopyHashLike(hashwrap::get(body, "a2"));
                py::object a3 = wrapper::deepCopyHashLike(hashwrap::get(body, "a3"));
                return py::make_tuple(a1, a2, a3);
            }

            py::tuple prepareTuple4(const karabo::data::Hash& body) {
                py::object a1 = wrapper::deepCopyHashLike(hashwrap::get(body, "a1"));
                py::object a2 = wrapper::deepCopyHashLike(hashwrap::get(body, "a2"));
                py::object a3 = wrapper::deepCopyHashLike(hashwrap::get(body, "a3"));
                py::object a4 = wrapper::deepCopyHashLike(hashwrap::get(body, "a4"));
                return py::make_tuple(a1, a2, a3, a4);
            }
        };


        class AsyncReplyWrap : public karabo::xms::SignalSlotable::AsyncReply {
           public:
            explicit AsyncReplyWrap(SignalSlotable* signalSlotable)
                : karabo::xms::SignalSlotable::AsyncReply(signalSlotable) {}

            void replyPy0() const {
                py::gil_scoped_release release;
                // Call inherited operator(..) - no GIL since that synchronously sends a message:
                (*this)();
            }

            void replyPy1(const py::object& a1) const {
                // Convert Python object to std::any - may involve a copy :-(
                std::any a1Any;
                wrapper::castPyToAny(a1, a1Any);
                // Call inherited operator(..) - no GIL since that synchronously sends a message:
                py::gil_scoped_release release;
                (*this)(a1Any);
            }

            void replyPy2(const py::object& a1, const py::object& a2) const {
                // Convert Python objects to std::any - may involve copies :-(
                std::any a1Any, a2Any;
                wrapper::castPyToAny(a1, a1Any);
                wrapper::castPyToAny(a2, a2Any);
                // Call inherited operator(..) - no GIL since that synchronously sends a message:
                py::gil_scoped_release release;
                (*this)(a1Any, a2Any);
            }

            void replyPy3(const py::object& a1, const py::object& a2, const py::object& a3) const {
                // Convert Python objects to std::any - may involve copies :-(
                std::any a1Any, a2Any, a3Any;
                wrapper::castPyToAny(a1, a1Any);
                wrapper::castPyToAny(a2, a2Any);
                wrapper::castPyToAny(a3, a3Any);
                // Call inherited operator(..) - no GIL since that synchronously sends a message:
                py::gil_scoped_release release;
                (*this)(a1Any, a2Any, a3Any);
            }

            void replyPy4(const py::object& a1, const py::object& a2, const py::object& a3,
                          const py::object& a4) const {
                // Convert Python objects to std::any - may involve copies :-(
                std::any a1Any, a2Any, a3Any, a4Any;
                wrapper::castPyToAny(a1, a1Any);
                wrapper::castPyToAny(a2, a2Any);
                wrapper::castPyToAny(a3, a3Any);
                wrapper::castPyToAny(a4, a4Any);
                // Call inherited operator(..) - no GIL since that synchronously sends a message:
                py::gil_scoped_release release;
                (*this)(a1Any, a2Any, a3Any, a4Any);
            }
        };

        AsyncReplyWrap createAsyncReply() {
            return SignalSlotableWrap::AsyncReplyWrap(this);
        }

       public:
        SignalSlotableWrap(const std::string& instanceId = generateInstanceId<SignalSlotable>(),
                           const karabo::data::Hash& connectionParameters = karabo::data::Hash(),
                           int heartbeatInterval = 10, const karabo::data::Hash& instanceInfo = karabo::data::Hash())
            : SignalSlotable(instanceId, connectionParameters, heartbeatInterval, instanceInfo) {}

        virtual ~SignalSlotableWrap() {}

        static std::shared_ptr<SignalSlotableWrap> create(
              const std::string& instanceId = generateInstanceId<SignalSlotable>(),
              const karabo::data::Hash& connectionParameters = karabo::data::Hash(), int heartbeatInterval = 10,
              const karabo::data::Hash& instanceInfo = karabo::data::Hash()) {
            return std::shared_ptr<SignalSlotableWrap>(
                  new SignalSlotableWrap(instanceId, connectionParameters, heartbeatInterval, instanceInfo));
        }

        void start() {
            py::gil_scoped_release release;
            try {
                karabo::xms::SignalSlotable::start();
            } catch (const std::exception& e) {
                // Make sure that we get something in the log file and not only in standard output/error
                KARABO_LOG_FRAMEWORK_ERROR << e.what();
                KARABO_RETHROW;
            }
        }

        void registerSlotPy(const py::object& slotFunction, std::string slotName, int numArgs) {
            if (slotName.empty()) {
                slotName = slotFunction.attr("__name__").cast<std::string>();
            }
            std::lock_guard<std::mutex> lock(m_signalSlotInstancesMutex);
            SlotInstances::const_iterator it = m_slotInstances.find(slotName);
            if (it != m_slotInstances.end()) { // Already registered
                (std::static_pointer_cast<SlotWrap>(it->second))->registerSlotFunction(slotFunction, numArgs);
            } else {
                std::shared_ptr<SlotWrap> s(std::make_shared<SlotWrap>(slotName));
                s->registerSlotFunction(slotFunction, numArgs); // Bind user's slot-function to Slot
                m_slotInstances[slotName] = s;
            }
        }

        template <typename... Args>
        void registerSignalPy(const std::string& funcName, const Args&... args) {
            // Arguments are ignored, but required to partially deduce the signature of the signal in Python:
            // All args will always be py::object, but at least the number of arguments defines the signal signature
            registerSignal<Args...>(funcName);
        }

        template <typename... Args>
        void registerSystemSignalPy(const std::string& funcName, const Args&... args) {
            // Arguments are ignored, but required to partially deduce the signature of the signal in Python:
            // All args will always be py::object, but at least the number of arguments defines the signal signature
            registerSystemSignal<Args...>(funcName);
        }

        template <typename... Args>
        SignalSlotableWrap::RequestorWrap requestPy(std::string instanceId, const std::string& functionName,
                                                    const Args&... args) {
            if (instanceId.empty()) instanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestPy(instanceId, functionName, args...);
        }

        template <typename... Args>
        SignalSlotableWrap::RequestorWrap requestNoWaitPy(std::string requestSlotInstanceId,
                                                          const std::string& requestSlotFunction,
                                                          std::string replySlotInstanceId,
                                                          const std::string& replySlotFunction, const Args&... args) {
            if (requestSlotInstanceId.empty()) requestSlotInstanceId = m_instanceId;
            if (replySlotInstanceId.empty()) replySlotInstanceId = m_instanceId;
            return SignalSlotableWrap::RequestorWrap(this).requestNoWaitPy(
                  requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction, args...);
        }

        template <typename... Args>
        void emitPy(const std::string& signalFunction, const Args&... args) {
            auto s = getSignal(signalFunction);
            if (s) {
                auto hash = std::make_shared<karabo::data::Hash>();
                packPy(*hash, args...);
                s->emit<Args...>(hash);
            }
        }

        template <typename... Args>
        void callPy(const std::string& instanceId, const std::string& functionName, const Args&... args) const {
            auto body = std::make_shared<karabo::data::Hash>();
            packPy(*body, args...);
            const std::string& id = (instanceId.empty() ? m_instanceId : instanceId);
            auto header = prepareCallHeader(id, functionName);
            doSendMessage(id, header, body, KARABO_SYS_PRIO, KARABO_SYS_TTL);
        }

        template <typename... Args>
        void replyPy(const Args&... args) {
            auto reply(std::make_shared<karabo::data::Hash>());
            packPy(*reply, args...);
            registerReply(reply);
        }

        karabo::xms::InputChannel::Pointer createInputChannelPy(const std::string& channelName,
                                                                const karabo::data::Hash& config,
                                                                const py::object& onDataHandler = py::none(),
                                                                const py::object& onInputHandler = py::none(),
                                                                const py::object& onEndOfStreamHandler = py::none(),
                                                                const py::object& connectionTracker = py::none()) {
            using namespace karabo::xms;
            // Basically just call createInputChannel from C++, but take care that data and input handlers
            // stay empty if their input is empty, although the proxies are able
            // to deal with 'None' Python handlers (as we make use of for the end of stream handler).
            DataHandler dataHandler = DataHandler();
            InputHandler inputHandler = InputHandler();
            InputHandler eosHandler = InputHandler();
            InputChannel::ConnectionTracker tracker = InputChannel::ConnectionTracker();

            if (onDataHandler != py::none()) { // or: if (onDataHandler.ptr() != Py_None) {
                dataHandler = InputChannelDataHandler(onDataHandler, "data");
            }
            if (onInputHandler != py::none()) {
                inputHandler = HandlerWrap<const karabo::xms::InputChannel::Pointer&>(onInputHandler, "input");
            }
            if (onEndOfStreamHandler != py::none()) {
                eosHandler = HandlerWrap<const karabo::xms::InputChannel::Pointer&>(onEndOfStreamHandler, "EOS");
            }
            if (connectionTracker != py::none()) {
                tracker = HandlerWrap<const std::string&, karabo::net::ConnectionStatus>(connectionTracker,
                                                                                         "channelStatusTracker");
            }

            return this->createInputChannel(channelName, config, dataHandler, inputHandler, eosHandler, tracker);
        }
    };

} // namespace karabind


using namespace karabo::data;
using namespace karabo::xms;
using namespace karabind;
using namespace std;


void exportPyXmsSignalSlotable(py::module_& m) {
    py::class_<SlotWrap, std::shared_ptr<SlotWrap>>(m, "Slot").def(
          "getInstanceIdOfSender", &SlotWrap::getInstanceIdOfSender, py::return_value_policy::reference_internal);

    py::class_<SignalSlotableWrap::RequestorWrap>(m, "Requestor")
          .def("waitForReply", &SignalSlotableWrap::RequestorWrap::waitForReply, py::arg("milliseconds"))

          .def("receiveAsync", &SignalSlotableWrap::RequestorWrap::receiveAsyncPy, py::arg("replyCallback"),
               py::arg("errorCallback") = py::none(), py::arg("timeoutMs") = py::none(),
               py::arg("numCallbackArgs") = py::none(),
               "Asynchronously receive the reply to the requested slot via a callback\n\n"
               "replyCallback   - called when slot reply arrives, argument number must match number replied by slot\n"
               "errorCallback   - if specified, called when the slot replies an error or the reply times out,\n"
               "                  will receive two str arguments:\n"
               "                  short description of the problem and details (e.g. stack trace)\n"
               "timeoutMs       - specify timeout in milliseconds (otherwise a long default is used)\n"
               "numCallbackArgs - only needed to specify if number of arguments cannot be deduced from replyCallback")
          // Keep the following (with number of return arguments in function name, but without error handling) for
          // backward compatibility
          .def("receiveAsync0", &SignalSlotableWrap::RequestorWrap::receiveAsyncPy0, (py::arg("replyCallback")),
               "Use receiveAsync instead")
          .def("receiveAsync1", &SignalSlotableWrap::RequestorWrap::receiveAsyncPy1, (py::arg("replyCallback")),
               "Use receiveAsync instead")
          .def("receiveAsync2", &SignalSlotableWrap::RequestorWrap::receiveAsyncPy2, (py::arg("replyCallback")),
               "Use receiveAsync instead")
          .def("receiveAsync3", &SignalSlotableWrap::RequestorWrap::receiveAsyncPy3, (py::arg("replyCallback")),
               "Use receiveAsync instead")
          .def("receiveAsync4", &SignalSlotableWrap::RequestorWrap::receiveAsyncPy4, (py::arg("replyCallback")),
               "Use receiveAsync instead");

    py::class_<SignalSlotableWrap::AsyncReplyWrap>(m, "AsyncReply")
          .def("__call__", &SignalSlotableWrap::AsyncReplyWrap::replyPy0, "Reply slot call without argument")
          .def("__call__", &SignalSlotableWrap::AsyncReplyWrap::replyPy1, py::arg("a1"),
               "Reply slot call with one argument")
          .def("__call__", &SignalSlotableWrap::AsyncReplyWrap::replyPy2, py::arg("a1"), py::arg("a2"),
               "Reply slot call with two arguments")
          .def("__call__", &SignalSlotableWrap::AsyncReplyWrap::replyPy3, py::arg("a1"), py::arg("a2"), py::arg("a3"),
               "Reply slot call with three arguments")
          .def("__call__", &SignalSlotableWrap::AsyncReplyWrap::replyPy4, py::arg("a1"), py::arg("a2"), py::arg("a4"),
               py::arg("a4"), "Reply slot call with four arguments")
          .def("error", &SignalSlotableWrap::AsyncReply::error, py::arg("message"), py::arg("details"),
               "Reply failure of slot call stating an error message and details like the\n"
               "stack trace from 'traceback.format_exc()'");

    py::class_<SignalSlotable, std::shared_ptr<SignalSlotable>>(m, "SignalSlotableIntern")
          .def(py::init<const std::string&, const karabo::net::Broker::Pointer&>());

    py::class_<SignalSlotableWrap, std::shared_ptr<SignalSlotableWrap>, SignalSlotable>(m, "SignalSlotable")
          .def(py::init<>())
          .def(py::init<const std::string&>())
          .def(py::init<const std::string&, const karabo::data::Hash&>())
          .def(py::init<const std::string&, const karabo::data::Hash&, const int>())
          .def(py::init<const std::string&, const karabo::data::Hash&, const int, const karabo::data::Hash&>())
          .def_static("create", &SignalSlotableWrap::create, py::arg("instanceId"),
                      py::arg("connectionParameters") = karabo::data::Hash(), py::arg("heartbeatInterval") = 10,
                      py::arg("instanceInfo") = karabo::data::Hash(),
                      "\nUse this factory method to create SignalSlotable object with given 'instanceId', "
                      "'connectionParameters', 'heartbeatInterval' and 'instanceInfo'.\n"
                      "Example:\n\tss = SignalSlotable.create('a')\n")
          .def("start", &SignalSlotableWrap::start)
          .def("getSenderInfo", &SignalSlotable::getSenderInfo, py::arg("slotFunction"),
               py::return_value_policy::reference_internal)
          .def("getInstanceId", &SignalSlotable::getInstanceId)
          .def("updateInstanceInfo", &SignalSlotable::updateInstanceInfo, py::arg("update"), py::arg("remove") = false)
          .def("getInstanceInfo", &SignalSlotable::getInstanceInfo)
          .def(
                "registerSlotCallGuardHandler",
                [](SignalSlotable& self, const py::object& handler) {
                    self.registerSlotCallGuardHandler(
                          HandlerWrap<const std::string&, const std::string&>(handler, "slot call guard"));
                },
                py::arg("handler"))
          .def(
                "registerPerformanceStatisticsHandler",
                [](SignalSlotable& self, const py::object& handler) {
                    self.registerPerformanceStatisticsHandler(
                          HandlerWrap<const Hash::Pointer&>(handler, "performance measurement"));
                },
                py::arg("handler"))
          .def(
                "registerBrokerErrorHandler",
                [](SignalSlotable& self, const py::object& handler) {
                    self.registerBrokerErrorHandler(HandlerWrap<const std::string&>(handler, "broker error"));
                },
                py::arg("handler"))
          .def(
                "connect",
                [](SignalSlotable& self, const std::string& signalInstanceId, const std::string& signalFunction,
                   const std::string& slotInstanceId, const std::string& slotFunction) {
                    py::gil_scoped_release release;
                    return self.connect(signalInstanceId, signalFunction, slotInstanceId, slotFunction);
                },
                py::arg("signalInstanceId"), py::arg("signalFunction"), py::arg("slotInstanceId"),
                py::arg("slotFunction"),
                "\nUse this method to connect \"signalFunction\" issued by \"signalInstanceId\" with \"slotFunction\" "
                "belonging to \"slotInstanceId\""
                "\n\nExample:\n\nIf we have to communicate with remote client registered as instance \"b\" and slot "
                "\"onMoin\" ...\n\n\t"
                "ss = SignalSlotable(\"a\")\n\tss.connect(\"\", \"moin\", \"b\", \"onMoin\")\n\tss.emit(\"moin\", "
                "12)\n")
          .def(
                "getAvailableInstances",
                [](SignalSlotable& self, bool activateTracking) {
                    return self.getAvailableInstances(activateTracking);
                },
                py::arg("activateTracking") = false)
          .def(
                "getAvailableSignals",
                [](SignalSlotable& self, const std::string& instanceId) {
                    return py::cast(self.getAvailableSignals(instanceId));
                },
                py::arg("instanceId"))
          .def(
                "getAvailableSlots",
                [](SignalSlotable& self, const std::string& instanceId) {
                    return py::cast(self.getAvailableSlots(instanceId));
                },
                py::arg("instanceId"))
          .def("disconnect", &SignalSlotable::disconnect, py::arg("signalInstanceId"), py::arg("signalFunction"),
               py::arg("slotInstanceId"), py::arg("slotFunction"))
          .def("getInstanceId", &SignalSlotable::getInstanceId, py::return_value_policy::reference_internal)
          .def("registerSlot", (&SignalSlotableWrap::registerSlotPy), py::arg("slotFunction"),
               py::arg("slotName") = std::string(), py::arg("numArgs") = -1,
               "Register a callable as slot function.\n"
               "If slotName empty (default), it is registered under slotFunction.__name__, else under slotName\n"
               "If numArg < 0 (default), try to deduce number of arguments that the function should take\n"
               "   - that is known to work for functions and methods without '*args'. Otherwise use numArgs.")
          .def("registerSignal", &SignalSlotableWrap::registerSignalPy<>, py::arg("signalFunction"))
          .def("registerSignal", &SignalSlotableWrap::registerSignalPy<py::object>, py::arg("signalFunction"),
               py::arg("a1"))
          .def("registerSignal", &SignalSlotableWrap::registerSignalPy<py::object, py::object>,
               py::arg("signalFunction"), py::arg("a1"), py::arg("a2"))
          .def("registerSignal", &SignalSlotableWrap::registerSignalPy<py::object, py::object, py::object>,
               py::arg("signalFunction"), py::arg("a1"), py::arg("a2"), py::arg("a3"))
          .def("registerSignal", &SignalSlotableWrap::registerSignalPy<py::object, py::object, py::object, py::object>,
               py::arg("signalFunction"), py::arg("a1"), py::arg("a2"), py::arg("a3"), py::arg("a4"))
          .def("registerSystemSignal", &SignalSlotableWrap::registerSystemSignalPy<>, py::arg("signalFunction"))
          .def("registerSystemSignal", &SignalSlotableWrap::registerSystemSignalPy<py::object>,
               py::arg("signalFunction"), py::arg("a1"))
          .def("registerSystemSignal", &SignalSlotableWrap::registerSystemSignalPy<py::object, py::object>,
               py::arg("signalFunction"), py::arg("a1"), py::arg("a2"))
          .def("registerSystemSignal", &SignalSlotableWrap::registerSystemSignalPy<py::object, py::object, py::object>,
               py::arg("signalFunction"), py::arg("a1"), py::arg("a2"), py::arg("a3"))
          .def("registerSystemSignal",
               &SignalSlotableWrap::registerSystemSignalPy<py::object, py::object, py::object, py::object>,
               py::arg("signalFunction"), py::arg("a1"), py::arg("a2"), py::arg("a3"), py::arg("a4"))
          .def("emit", &SignalSlotableWrap::emitPy<>, py::arg("signalFunction"))
          .def("emit", &SignalSlotableWrap::emitPy<py::object>, py::arg("signalFunction"), py::arg("a1"))
          .def("emit", &SignalSlotableWrap::emitPy<py::object, py::object>, py::arg("signalFunction"), py::arg("a1"),
               py::arg("a2"))
          .def("emit", &SignalSlotableWrap::emitPy<py::object, py::object, py::object>, py::arg("signalFunction"),
               py::arg("a1"), py::arg("a2"), py::arg("a3"))
          .def("emit", &SignalSlotableWrap::emitPy<py::object, py::object, py::object, py::object>,
               py::arg("signalFunction"), py::arg("a1"), py::arg("a2"), py::arg("a3"), py::arg("a4"))
          .def("call", &SignalSlotableWrap::callPy<>, py::arg("instanceId"), py::arg("slotName"))
          .def("call", &SignalSlotableWrap::callPy<py::object>, py::arg("instanceId"), py::arg("slotName"),
               py::arg("a1"))
          .def("call", &SignalSlotableWrap::callPy<py::object, py::object>, py::arg("instanceId"), py::arg("slotName"),
               py::arg("a1"), py::arg("a2"))
          .def("call", &SignalSlotableWrap::callPy<py::object, py::object, py::object>, py::arg("instanceId"),
               py::arg("slotName"), py::arg("a1"), py::arg("a2"), py::arg("a3"))
          .def("call", &SignalSlotableWrap::callPy<py::object, py::object, py::object, py::object>,
               py::arg("instanceId"), py::arg("slotName"), py::arg("a1"), py::arg("a2"), py::arg("a3"), py::arg("a4"))
          .def("request", &SignalSlotableWrap::requestPy<>, py::arg("instanceId"), py::arg("slotName"))
          .def("request", &SignalSlotableWrap::requestPy<py::object>, py::arg("instanceId"), py::arg("slotName"),
               py::arg("a1"))
          .def("request", &SignalSlotableWrap::requestPy<py::object, py::object>, py::arg("instanceId"),
               py::arg("slotName"), py::arg("a1"), py::arg("a2"))
          .def("request", &SignalSlotableWrap::requestPy<py::object, py::object, py::object>, py::arg("instanceId"),
               py::arg("slotName"), py::arg("a1"), py::arg("a2"), py::arg("a3"))
          .def("request", &SignalSlotableWrap::requestPy<py::object, py::object, py::object, py::object>,
               py::arg("instanceId"), py::arg("slotName"), py::arg("a1"), py::arg("a2"), py::arg("a3"), py::arg("a4"))

          .def("requestNoWait", &SignalSlotableWrap::requestNoWaitPy<>, py::arg("requestInstanceId"),
               py::arg("requestSlotName"), py::arg("replyInstanceId"), py::arg("replySlotName"))
          .def("requestNoWait", &SignalSlotableWrap::requestNoWaitPy<py::object>, py::arg("requestInstanceId"),
               py::arg("requestSlotName"), py::arg("replyInstanceId"), py::arg("replySlotName"), py::arg("a1"))
          .def("requestNoWait", &SignalSlotableWrap::requestNoWaitPy<py::object, py::object>,
               py::arg("requestInstanceId"), py::arg("requestSlotName"), py::arg("replyInstanceId"),
               py::arg("replySlotName"), py::arg("a1"), py::arg("a2"))
          .def("requestNoWait", &SignalSlotableWrap::requestNoWaitPy<py::object, py::object, py::object>,
               py::arg("requestInstanceId"), py::arg("requestSlotName"), py::arg("replyInstanceId"),
               py::arg("replySlotName"), py::arg("a1"), py::arg("a2"), py::arg("a3"))
          .def("requestNoWait", &SignalSlotableWrap::requestNoWaitPy<py::object, py::object, py::object, py::object>,
               py::arg("requestInstanceId"), py::arg("requestSlotName"), py::arg("replyInstanceId"),
               py::arg("replySlotName"), py::arg("a1"), py::arg("a2"), py::arg("a3"), py::arg("a4"))
          .def("reply", &SignalSlotableWrap::replyPy<>)
          .def("reply", &SignalSlotableWrap::replyPy<py::object>, py::arg("a1"))
          .def("reply", &SignalSlotableWrap::replyPy<py::object, py::object>, py::arg("a1"), py::arg("a2"))
          .def("reply", &SignalSlotableWrap::replyPy<py::object, py::object, py::object>, py::arg("a1"), py::arg("a2"),
               py::arg("a3"))
          .def("reply", &SignalSlotableWrap::replyPy<py::object, py::object, py::object, py::object>, py::arg("a1"),
               py::arg("a2"), py::arg("a3"), py::arg("a4"))
          .def("createAsyncReply", &SignalSlotableWrap::createAsyncReply,
               "Create an AsyncReply to postpone the reply of a slot.\n\n"
               "Only call within a slot call - and then take care to use the AsyncReply to\n"
               "either reply or report an error since no automatic reply will happen.")
          .def(
                "createOutputChannel",
                [](SignalSlotable& self, const std::string& channelName, const Hash& config,
                   const py::object& onOutput) {
                    SignalSlotable::OutputHandler out = SignalSlotable::OutputHandler();
                    if (onOutput != py::none()) {
                        out = HandlerWrap<const karabo::xms::OutputChannel::Pointer&>(onOutput, "IOEvent");
                    }
                    return self.createOutputChannel(channelName, config, out);
                },
                py::arg("channelName"), py::arg("configuration"), py::arg("handler") = py::none())
          .def("createInputChannel", &SignalSlotableWrap::createInputChannelPy, py::arg("channelName"),
               py::arg("configuration"), py::arg("onData") = py::none(), py::arg("onInput") = py::none(),
               py::arg("onEndOfStream") = py::none(), py::arg("connectionTracker") = py::none())
          .def("connectInputChannels",
               [](SignalSlotable& self) {
                   py::gil_scoped_release release;
                   self.connectInputChannels(boost::system::error_code());
               })
          .def("getOutputChannel", &SignalSlotable::getOutputChannel, py::arg("channelName"))
          .def("getInputChannel", &SignalSlotable::getInputChannel, py::arg("channelName"))
          .def("getOutputChannelNames", &SignalSlotable::getOutputChannelNames)
          .def("getInputChannelNames",
               [](SignalSlotable& self) -> py::list {
                   py::list result;
                   for (const auto& inputNameChannel : self.getInputChannels()) {
                       result.append(inputNameChannel.first);
                   }
                   return result;
               })
          .def("removeInputChannel", &SignalSlotable::removeInputChannel, py::arg("channelName"))
          .def("removeOutputChannel", &SignalSlotable::removeOutputChannel, py::arg("channelName"))
          .def(
                "registerDataHandler",
                [](SignalSlotable& self, const std::string& channelName, const py::object& handler) {
                    SignalSlotable::DataHandler dataHandler = SignalSlotable::DataHandler();
                    if (handler != py::none()) {
                        dataHandler = InputChannelDataHandler(handler, "data");
                    }
                    self.registerDataHandler(channelName, dataHandler);
                },
                py::arg("channelName"), py::arg("handlerPerData") = py::none())
          .def(
                "registerInputHandler",
                [](SignalSlotable& self, const std::string& channelName, const py::object& handler) {
                    SignalSlotable::InputHandler inputHandler = SignalSlotable::InputHandler();
                    if (handler != py::none()) {
                        inputHandler = HandlerWrap<const InputChannel::Pointer&>(handler, "input");
                    }
                    self.registerInputHandler(channelName, inputHandler);
                },
                py::arg("channelName"), py::arg("handlerPerInput") = py::none())
          .def(
                "registerEndOfStreamHandler",
                [](SignalSlotable& self, const std::string& channelName, const py::object& handler) {
                    SignalSlotable::InputHandler eos = SignalSlotable::InputHandler();
                    if (handler != py::none()) {
                        eos = HandlerWrap<const InputChannel::Pointer&>(handler, "EOS");
                    }
                    self.registerEndOfStreamHandler(channelName, eos);
                },
                py::arg("channelName"), py::arg("handler") = py::none())
          .def(
                "exists",
                [](SignalSlotable& self, const std::string& instanceId) -> py::tuple {
                    std::pair<bool, std::string> result;
                    {
                        py::gil_scoped_release release;
                        result = self.exists(instanceId);
                    }
                    return py::make_tuple(result.first, py::cast(result.second));
                },
                py::arg("instanceId"))
          .def("getConnection", &SignalSlotable::getConnection)
          .def("getTopic", &SignalSlotable::getTopic, py::return_value_policy::reference_internal);
}
