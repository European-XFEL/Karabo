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
/*
 * File:   AmqpClient.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on May 18, 2021, 1:18 PM
 */

#ifndef KARABO_NET_AMQPCLIENT_HH
#define KARABO_NET_AMQPCLIENT_HH

#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>

#include <boost/asio.hpp>
#include <boost/system/system_error.hpp>
#include <condition_variable>

#include "AmqpUtils.hh"
#include "EventLoop.hh" // AsyncHandler
#include "Strand.hh"
#include "karabo/io/BinarySerializer.hh"
#include "karabo/util/Configurator.hh" // KARABO_CONFIGURATION_BASE_CLASS
#include "utils.hh"


namespace karabo {
    namespace net {


        class AmqpConnector;

        typedef std::function<void(const AMQP::Message& message, uint64_t deliveryTag, bool redelivered)>
              AmqpCppMessageCallback;

        //------------------------------------------------------------------------------------ AmqpTransceiver

        /**
         * @brief Transceiver representing communication point with AMQP broker. It contains AMQP channel,
         * exchange and, possibly, queue, routingKey, listener flag.  The transceiver after creation should
         * be activated i.e. started by creating a channel, declaring exchange, queue and binding them.
         * This is implemented by running state machine defining order of operation.
         *
         * Inspired by and many thanks to https://github.com/cycleg/amqp-cpp-asio
         */
        class AmqpTransceiver : public boost::enable_shared_from_this<AmqpTransceiver> {
            friend class AmqpConnector;

           public:
            typedef boost::shared_ptr<AmqpTransceiver> Pointer;

            AmqpTransceiver(const std::string& exchange, const std::string& queue_, const std::string& route_in,
                            bool listener, const AMQP::Table& queueArgs);

            virtual ~AmqpTransceiver();

            AmqpTransceiver(const AmqpTransceiver&) = delete;

            inline const std::string& exchangePoint() const {
                return m_exchange;
            }

            inline const std::string& queueName() const {
                return m_queue;
            }

            inline const std::string& recvQueueName() const {
                return m_recvQueue;
            }

            inline const std::string& routingKey() const {
                return m_route;
            }

            inline bool isListener() const {
                return m_listener;
            }

            /**
             * @brief predicate informing if transceiver was started
             *
             * @return true  started
             * @return false not started
             */
            inline bool isRunning() const {
                return m_state != eEnd;
            }

            /**
             * @brief Is transceiver busy going to 'ready' or 'end' state?
             *
             * @return true
             * @return false
             */
            inline bool isBusy() const {
                return (m_state != eReady && m_state != eEnd);
            }

            /**
             * @brief predicate reporting if tranceiver is in 'ready' state i.e. ready to operate
             *
             * @return true
             * @return false
             */
            inline bool ready() const {
                return m_connection && m_connection->usable() && m_state == eReady;
            }

            /**
             * @brief Reporting error_code in custom classification
             *
             * @return boost::system::error_code
             */
            inline boost::system::error_code errorCode() const {
                return m_ec;
            }

            /**
             * @brief reporting detailed error message from AMQPCPP library
             *
             * @return std::string
             */
            inline std::string error() const {
                return m_error;
            }

            /**
             * @brief callback for incoming messages from queue
             *
             * @param callback
             */
            void onMessage(AmqpCppMessageCallback&& callback);

            /**
             * @brief transceiver sending message to a broker
             *
             * @param message     to be sent
             * @param route       routinKey for broker to dispatch this message
             * @param onComplete  callback when completed
             */
            void sendAsync(const std::shared_ptr<std::vector<char>>& message, const std::string& route,
                           const AsyncHandler& onComplete);

           private:
            enum State {
                eCreateChannel,   // Create a channel
                eCheckQueue,      // Check if broker has already the queue
                                  // for incoming messages
                eRecreateChannel, // Re-create channel.
                eCreateExchange,  // Create or open exchange point
                eCreateQueue,     // Create or open the queue for incoming messages
                eBindQueue,       // Queue is bound to exchange
                eCreateConsumer,  // initialize consumer
                eReady,           // ready for publish/consume
                eShutdown,        // Begin state machine stopping
                eUnbindQueue,     // Unbind exchange and queue
                // eRemoveQueue,     // Remove queue
                //  Exchange point, if no queues bound to, is automatically
                //  deleted at channel closure
                eCloseChannel, // Close AMQP channel.
                eEnd,          // State machine is finished. It is a default.
                eMax
            };

            static const int s_exchangeCreationFlags; // Flags used at exchange creation

            static std::unordered_map<State, State> s_stopTransit; // Transfer table to
                                                                   // stop client

            friend inline std::ostream& operator<<(std::ostream& out, const State& s) {
                static const char* str[eMax + 1] = {"eCreateChannel",
                                                    "eCheckQueue",
                                                    "eRecreateChannel",
                                                    "eCreateExchange",
                                                    "eCreateQueue",
                                                    "eBindQueue",
                                                    "eCreateConsumer",
                                                    "eReady",
                                                    "eShutdown",
                                                    "eUnbindQueue",
                                                    "eCloseChannel",
                                                    "eEnd",
                                                    "eMax"};
                out << str[s];
                return out;
            }

            void _sendAsync(const std::shared_ptr<std::vector<char>>& message, const std::string& route,
                            const AsyncHandler& onComplete);

            /**
             * @brief start transceiver asynchronously following the steps:
             * creating channel
             * check and/or creating queue if listener flag is true
             * creating exchange
             * bind exchange and queue if listener flag is true
             *
             * @param connector  parent connector that this transceiver belongs to
             * @param recvQueue  the queue name registered already on the broker or empty string
             * @param onComplete completion callback function object (functor)
             */
            void startAsync(const boost::shared_ptr<AmqpConnector>& connector, const std::string& recvQueue,
                            const AsyncHandler& onComplete);

            void _startAsync(const boost::shared_ptr<AmqpConnector>& connector, const std::string& recvQueue,
                             const AsyncHandler& onComplete);

            /**
             * @brief deactivating tranceiver asynchronously
             *
             * @param onComplete
             */
            void stopAsync(const AsyncHandler& onComplete);

            void _stopAsync(const AsyncHandler& onComplete);

            /**
             * @brief transceiver dropped synchronously
             *
             * @return boost::system::error_code
             */
            boost::system::error_code drop();

            /**
             * @brief move state machine to the programmed states until it exits
             *
             */
            void moveStateMachine();

            friend inline std::ostream& operator<<(std::ostream& out, const AmqpTransceiver& s) {
                out << "Transceiver exchange=\"" << s.m_exchange << "\", queue=\"" << s.m_queue << "\", recvQueue=\""
                    << s.m_recvQueue << "\", route=\"" << s.m_route << "\",\n\t\tconsumerTag=\"" << s.m_consumerTag
                    << "\", listener=" << s.m_listener << ", state=" << s.m_state;
                return out;
            }

           private:
            State m_state;                              // current state
            boost::weak_ptr<AmqpConnector> m_connector; // pointer to the parent - weak_ptr to avoid cyclic reference
                                                        // (if empty, parent is [being] destructed, so safe to bail out)
            std::string m_exchange;                     // echange point
            std::string m_queue;                        // queue name for incoming messages
            std::string m_recvQueue;                    // queue name received from broker
            std::string m_route;                        // routing (binding) key
            std::string m_consumerTag;                  // subscriber tag on the AMQP broker
            bool m_listener;                            // flag indicating that transceiver is a receiver
            bool m_queueExist;                          // flag indicating that queue is known on the broker
            AmqpCppMessageCallback m_onMessage;         // callback for incoming messages
            AMQP::TcpConnection* m_connection;          // AMQP tcp connection raw pointer
            std::shared_ptr<AMQP::TcpChannel> m_channel; // channel associated with transceiver
            std::string m_error;                         // last detailed error message
            boost::system::error_code m_ec;              // error code
            std::list<AsyncHandler> m_completeHandlers;  // Current list of complete handlers
            std::mutex m_completeHandlersMutex;          // Protect the list
            AMQP::Table m_queueArgs;                     // arguments for AMQP queue
        };


        //-------------------------------------------------------------------------------- AmqpSingletonImpl

        /**
         * @brief Contains connection data and tries to reconnect if current connection is lost.
         */
        class AmqpSingletonImpl : public boost::enable_shared_from_this<AmqpSingletonImpl> {
            enum State {
                eNotConnected = 2000, // No connection
                eConnectionDone,      // Phys.connection done
                eConnectionReady,     // Logical connection (phys. + login)
                eConnectionClosed,    // Connection just closed
                eConnectionError,     // Connection error (TCP?) with error message
                eConnectionLost,      // Connection lost  (cluster node is shut down)
            };

            /// @brief Reference into private io_context instance
            boost::asio::io_context& m_ioctx;

            /// @brief Custom connection handler required by AMQP-CPP library
            std::shared_ptr<ConnectionHandler> m_handler;

            /// @brief Shared pointer to TcpConnection instance
            std::shared_ptr<AMQP::TcpConnection> m_connection;

            /// @brief The list of weak pointers to AmqpConnector instances
            std::set<boost::weak_ptr<AmqpConnector>> m_connectors;

            /// @brief  Mutex to peortect an access to the connector's list
            std::mutex m_connectorsMutex;

            /// @brief Current broker list with the first item being connected to
            std::list<std::string> m_urls;

            /// @brief Current state for connection process
            State m_state;

            /// @brief Error message for cconnection attempt
            std::string m_error;

            /// @brief Countdown counter for connection attempts
            int m_attemptCounter;

            /// @brief Current completion handler for connection
            AsyncHandler m_onComplete;

            /// @brief  Flag controlling blocking on condition variable
            bool m_stopWaiting;

            /// @brief Mutex for protecting access to condition variable
            std::mutex m_waitingMutex;

            /// @brief Condition variable for blocking sender's
            std::condition_variable m_activateCondition;

           private:
            AmqpSingletonImpl(const AmqpSingletonImpl&) = delete;

            /**
             * It attempts to connect (asynchronously) to the first entry in the broker list.
             * Call onComplete callback if succeeded, otherwise shift in the list to the next node
             * to try again via event loop. It is cycling until connection attempt is successful.
             * Later, in case of problems with connection this function will be called automatically
             * with the 'null' callback.
             *
             * @param onComplete
             */
            void connectAsync(const AsyncHandler& onComplete);

            /**
             * @brief Ask every controlled connector to drop its transceivers
             *
             * @param message describing the error associated with this request
             */
            void notifyConnectorsToDrop(const std::string& message);

            /**
             * @brief Ask every controlled connector to activate ('start') its transceivers
             *
             */
            void notifyConnectorsToStart();

            void showConnectorsStatus();

           public:
            explicit AmqpSingletonImpl(boost::asio::io_context& io_context);

            virtual ~AmqpSingletonImpl();

            /**
             * @brief This function called only once initiates connection to the cluster nodes. The function
             * is synchronous. It calls 'connectAsync(...)' and blocks until completion handler is called.
             *
             * @param urls list of node addresses in broker's cluster
             */
            void autoReconnect(const std::list<std::string>& urls);

            /**
             * @brief Get the Connection object
             *
             * @return const std::shared_ptr<AMQP::TcpConnection>&
             */
            const std::shared_ptr<AMQP::TcpConnection>& getConnection() const noexcept {
                return m_connection;
            }

            /**
             * @brief predicate returning a condition if the connection is established including
             * physical connection and login
             *
             * @return true
             * @return false
             */
            bool ready() {
                return (m_connection && m_handler && m_connection->usable());
            }

            /**
             * @brief returns succeeded URL from the broker list
             *
             * @return std::string
             */
            std::string url() const noexcept {
                return m_urls.front();
            }

            /**
             * @brief the list of broker candidates in some order
             *
             * @return const std::vector<std::string>&
             */
            const std::list<std::string>& urls() const noexcept {
                return m_urls;
            }

            /**
             * @brief gives an access to io_context (io_service)
             *
             * @return boost::asio::io_context&
             */
            boost::asio::io_context& ioContext() noexcept {
                return m_ioctx;
            }

            /**
             * @brief helper allowing post on event loop
             *
             * @tparam CompletionToken
             * @param token
             */
            template <typename CompletionToken>
            void post(CompletionToken&& token) {
                boost::asio::post(m_ioctx, std::forward<CompletionToken>(token));
            }


            /**
             * @brief Add 'connector' to the list of controlled connectors
             * They are kept in the list as a weak pointers.  This is called
             * if connector has at least one transceiver.
             *
             * @param connector shared pointer to the 'AmqpConnector' objrct
             */
            void registerConnector(const boost::shared_ptr<AmqpConnector>& connector);

            /**
             * @brief Remove connector from the list of controlled connectors.  This is
             * called if the tranceiver's list is emptied.
             *
             * @param connector shared pointer to the 'AmqpConnector' object
             */
            void unregisterConnector(const boost::shared_ptr<AmqpConnector>& connector);

            /**
             * @brief Predicate indicating that all connectors are started successfully
             * Connector is considered 'started' or 'activated' if it's transceivers are started.
             * It is important during broker re-connection process
             *
             * @return true  if all
             * @return false
             */
            bool allConnectorsStarted();

            /**
             * @brief Wait for  the state when all (registered!) connectors are started (activated).
             *
             */
            void waits();

            /**
             * @brief Wake up all threads blocked on condition variable
             *
             */
            void notifyAll();

            /**
             * @brief close AMQP connection
             *
             */
            void close();

            void onAttachedCallback(AMQP::TcpConnection*, const std::string& url);

            void onConnectedCallback(AMQP::TcpConnection*, const std::string& url);

            void onReadyCallback(AMQP::TcpConnection* connection, const std::string& url);

            void onErrorCallback(AMQP::TcpConnection* connection, const char* message, const std::string& url);

            void onClosedCallback(AMQP::TcpConnection*, const std::string& url);

            void onLostCallback(AMQP::TcpConnection*);

            void onDetachedCallback(AMQP::TcpConnection* connection);

            void onWaitCallback(const boost::system::error_code& ec,
                                const std::shared_ptr<boost::asio::deadline_timer>&);
        };


        //--------------------------------------------------------------------------------------- AmqpService


        /// @brief Make our AMQP broker connection to be a service following the pattern
        /// used in "logger" example in boost asio distribution, for example...
        /// https://www.boost.org/doc/libs/1_80_0/doc/html/boost_asio/example/cpp03/services/logger_service.hpp
        /// Our service is controlling a "private" io_context used for communications with AMQP broker and
        /// as a service goes down when the main EventLoop is stopped.

        class AmqpService : public boost::asio::execution_context::service {
           public:
            /// The type used to identify this service in the execution context.
            typedef AmqpService key_type;

            /// The type for an implementation of AMQP broker connection.
            typedef AmqpSingletonImpl* impl_type;

            /// @brief Constructor create a thread to run a private io_context.
            /// @param context
            AmqpService(boost::asio::execution_context& context)
                : boost::asio::execution_context::service(context),
                  m_iocontext(std::make_shared<boost::asio::io_context>()),
                  m_sentinel(boost::asio::make_work_guard(*m_iocontext)),
                  m_thread(std::make_unique<std::thread>([ctx{m_iocontext}]() { ctx->run(); })),
                  m_singletonMutex(),
                  m_singleton() {}

            /// Destructor shuts down the private io_context (in AmqpSingleton destructor)
            ~AmqpService() {
                m_sentinel.reset();
                if (m_thread) m_thread->join();
            }

            /// Destroy all user-defined handler objects owned by the service.
            void shutdown() {
                m_singleton.reset();
            }

            /// Return a null AMQP broker connection implementation
            impl_type null() const {
                return nullptr;
            }

            /// Create a new AMQPbroker connection's implementation.
            void create(impl_type& impl, const std::vector<std::string>& urls) {
                if (!m_singleton || !m_singleton->ready()) {
                    // try to lock weak pointer under mutex protection
                    std::lock_guard<std::mutex> lk(m_singletonMutex);
                    if (!m_singleton || !m_singleton->ready()) {
                        // create AmqpSingletonImpl object and connect it ...
                        m_singleton.reset(new AmqpSingletonImpl(*m_iocontext));
                        std::list<std::string> brokers(urls.begin(), urls.end());
                        m_singleton->autoReconnect(brokers);
                    }
                }
                impl = m_singleton.get();
            }

            /// Destroy AMQP broker connection implementation.
            void destroy(impl_type& impl) {
                impl = null();
            }

            const std::shared_ptr<AMQP::TcpConnection>& getConnection(const impl_type& impl) const {
                return impl->getConnection();
            }

            bool ready(const impl_type& impl) {
                return impl->ready();
            }

            /**
             * @brief returns succeeded URL from the broker list
             *
             * @return std::string
             */
            std::string url(const impl_type& impl) const {
                return impl->url();
            }

            /**
             * @brief the list of broker candidates in some order
             *
             * @return const std::vector<std::string>&
             */
            const std::list<std::string>& urls(const impl_type& impl) const {
                return impl->urls();
            }

            /**
             * @brief gives an access to io_context (io_service)
             *
             * @return boost::asio::io_context&
             */
            boost::asio::io_context& ioContext(impl_type& /*impl*/) {
                return *m_iocontext;
            }

            /**
             * @brief helper allowing post on event loop
             *
             * @tparam CompletionToken
             * @param token
             */
            template <typename CompletionToken>
            void post(CompletionToken&& token) {
                boost::asio::post(*m_iocontext, std::forward<CompletionToken>(token));
            }

            /**
             * @brief Add 'connector' to the list of controlled connectors
             * They are kept in the list as a weak pointers.  This is called
             * if connector has at least one transceiver.
             *
             * @param connector shared pointer to the 'AmqpConnector' objrct
             */
            void registerConnector(impl_type& impl, const boost::shared_ptr<AmqpConnector>& connector) {
                impl->registerConnector(connector);
            }

            /**
             * @brief Remove connector from the list of controlled connectors.  This is
             * called if the tranceiver's list is emptied.
             *
             * @param connector shared pointer to the 'AmqpConnector' object
             */
            void unregisterConnector(impl_type& impl, const boost::shared_ptr<AmqpConnector>& connector) {
                impl->unregisterConnector(connector);
            }

            /**
             * @brief Predicate indicating that all connectors are started successfully
             * Connector is considered 'started' or 'activated' if it's transceivers are
             * started. It is important during broker re-connection process
             *
             * @return true  if all
             * @return false
             */
            bool allConnectorsStarted(impl_type& impl) {
                return impl->allConnectorsStarted();
            }

            /**
             * @brief Wait for  the state when all (registered!) connectors are started
             * (activated).
             *
             */
            void waits(impl_type& impl) {
                impl->waits();
            }

            /**
             * @brief Wake up all threads blocked on condition variable
             *
             */
            void notifyAll(impl_type& impl) {
                impl->notifyAll();
            }

            /**
             * @brief close AMQP connection
             *
             */
            void close(impl_type& impl) {
                impl->close();
            }

           private:
            /// Private io_context used for performing operations with AMQP broker
            std::shared_ptr<boost::asio::io_context> m_iocontext;

            /// Work for private io_context to perform making sure the io_context.run()
            /// does not exit
            boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_sentinel;

            /// Thread used for running the work io_context's run loop.
            std::unique_ptr<std::thread> m_thread;

            /// Mutex to protect multiple access while creating singleton
            std::mutex m_singletonMutex;

            /// @brief Keeps connection to AMQP broker
            boost::shared_ptr<AmqpSingletonImpl> m_singleton;
        };


        //-------------------------------------------------------------------------------- AmqpSingletonBase


        /// @brief Register our service in EventLoop releated io_context
        /// @tparam Service
        template <typename Service>
        class AmqpSingletonBase : private boost::noncopyable {
           public:
            /// The type of the service that will be used to provide timer operations.
            typedef Service service_type;

            /// The native implementation type of the AMQP btoker connection.
            typedef typename service_type::impl_type impl_type;

            explicit AmqpSingletonBase(boost::asio::execution_context& context, const std::vector<std::string>& urls)
                : m_service(boost::asio::use_service<Service>(context)), m_impl(m_service.null()) {
                m_service.create(m_impl, urls);
            }

            ~AmqpSingletonBase() {
                m_service.destroy(m_impl);
            }

            const std::shared_ptr<AMQP::TcpConnection>& getConnection() const {
                return m_service.getConnection(m_impl);
            }

            bool ready() {
                return m_service.ready(m_impl);
            }

            std::string url() const {
                return m_service.url(m_impl);
            }

            const std::list<std::string>& urls() const {
                return m_service.urls(m_impl);
            }

            boost::asio::io_context& ioContext() {
                return m_service.ioContext(m_impl);
            }

            template <typename CompletionToken>
            void post(CompletionToken&& token) {
                boost::asio::post(m_service.ioContext(m_impl), std::forward<CompletionToken>(token));
            }

            void registerConnector(const boost::shared_ptr<AmqpConnector>& connector) {
                m_service.registerConnector(m_impl, connector);
            }

            void unregisterConnector(const boost::shared_ptr<AmqpConnector>& connector) {
                m_service.unregisterConnector(m_impl, connector);
            }

            bool allConnectorsStarted() {
                return m_service.allConnectorsStarted(m_impl);
            }

            void waits() {
                m_service.waits(m_impl);
            }

            void notifyAll() {
                m_service.notifyAll(m_impl);
            }

            void close() {
                m_service.close(m_impl);
            }

           private:
            /// The backend service implementation.
            service_type& m_service;

            /// The underlying native implementation.
            impl_type m_impl;
        };


        //------------------------------------------------------------------------------------ AmqpSingleton


        typedef AmqpSingletonBase<AmqpService> AmqpSingleton;


        //------------------------------------------------------------------------------------ AmqpConnector


        /**
         * @brief Contains connection object and history list of transcevers that should be activated again if
         * the connection object is re-created
         */
        class AmqpConnector : public boost::enable_shared_from_this<AmqpConnector> {
           public:
            typedef boost::shared_ptr<AmqpConnector> Pointer;

           private:
            // Input list of possible broker's list
            std::vector<std::string> m_urls;
            // Possible IDs in case of SignalSlotable's <instanceId>:
            // <instanceId>         -- regular instanceId
            // <instanceId>:beats   -- for reading heartbeats, for instance, GuiServer
            std::string m_instanceId;
            std::shared_ptr<AMQP::TcpChannel> m_publisher; // Shared channel for publishing
            std::unordered_map<std::string, AmqpTransceiver::Pointer> m_transceivers;
            // Mutex protecting access to `m_transceivers' container
            std::mutex m_transceiversMutex;
            // Flag that indicates that all transceivers are successfully started
            bool m_connectorActivated;
            std::mutex m_activateMutex;
            // Shared pointer to AmqpSingleton singleton (AMQP broker connection)
            std::shared_ptr<AmqpSingleton> m_amqp;

            AMQP::Table m_queueArgs;

            AmqpConnector() = delete;
            AmqpConnector(const AmqpConnector&) = delete;


           public:
            explicit AmqpConnector(const std::vector<std::string>& urls, const std::string& id,
                                   const AMQP::Table& queueArgs);
            virtual ~AmqpConnector();

            const std::shared_ptr<AmqpSingleton>& getSingleton() const noexcept {
                return m_amqp;
            }

            const std::shared_ptr<AMQP::TcpChannel>& getPublisher() const noexcept {
                return m_publisher;
            }

            void setPublisher(const std::shared_ptr<AMQP::TcpChannel>& channel) noexcept {
                m_publisher = channel;
            }

            template <typename CompletionToken>
            void post(CompletionToken&& token) {
                m_amqp->post(std::forward<CompletionToken>(token));
            }

            const std::string getRecvQueueName();

            /**
             * @brief predicate returning connection status
             *
             * @return true
             * @return false
             */
            bool ready() {
                return (m_amqp && m_amqp->ready());
            }

            /**
             * @brief
             *
             * @return true all transceivers are started
             * @return false some transceivers are not started yet
             */
            bool isActivated() {
                std::lock_guard<std::mutex> lock(m_activateMutex);
                return m_connectorActivated;
            }

            const std::string& id() const noexcept {
                return m_instanceId;
            }

            /**
             * @brief returns connected URL
             *
             * @return const std::string&
             */
            std::string url() noexcept {
                return m_amqp->url();
            }

            /**
             * @brief returns broker list
             *
             * @return const std::vector<std::string>&
             */
            const std::vector<std::string>& urls() const noexcept {
                return m_urls;
            }

            /**
             * @brief Get the Context object: io_context reference
             *
             * @return boost::asio::io_context&
             */
            boost::asio::io_context& getContext() noexcept {
                return m_amqp->ioContext();
            }

           private:
            /**
             * @brief creates passive transceiver object and add it into the Hash store
             *
             * @param exchange
             * @param queue
             * @param bindingKey
             * @param listenerFlag
             * @return AmqpTransceiver::Pointer
             */
            AmqpTransceiver::Pointer transceiver(const std::string& exchange, const std::string& queue,
                                                 const std::string& bindingKey, bool listenerFlag);

            std::string transceiverKey(const std::string& exchange, const std::string& bindingKey, bool listener) {
                std::string routingKey = listener ? bindingKey : "";
                return (exchange + "^" + routingKey);
            }

            void _onOpen(const boost::system::error_code& ec, std::queue<std::string> paths,
                         const AsyncHandler& onComplete);

            void _onClose(const boost::system::error_code& ec, std::queue<std::string> paths,
                          const AsyncHandler& onComplete);

            void sendDelayedCallback(const boost::system::error_code& ec, const std::string& exchange,
                                     const std::string& routingKey, const std::shared_ptr<std::vector<char>>& data,
                                     const AsyncHandler& onComplete,
                                     const std::shared_ptr<boost::asio::deadline_timer>& timer);

            void sendAsyncDelayed(const std::string& exchange, const std::string& routingKey,
                                  const std::shared_ptr<std::vector<char>>& data, const AsyncHandler& onComplete);

            /**
             * @brief activate transceiver asynchronously. Can be called several times
             *
             * @param p
             * @param onComplete
             */
            void open(const AmqpTransceiver::Pointer& p, const AsyncHandler& onComplete);

            /**
             * @brief deactivate transceiver asynchronously
             *
             * @param p
             * @param onComplete
             */
            void close(const AmqpTransceiver::Pointer& p, const AsyncHandler& onComplete);

           public:
            /**
             * @brief predicate to check if the transceiver with such parameters is created already
             *
             * @param exchange
             * @param routingKey
             * @param listener
             * @return true
             * @return false
             */
            bool has(const std::string& exchange, const std::string& routingKey, bool listener);

            /**
             * @brief send message to the broker using given paraters asynchronously
             *
             * @param exchange
             * @param routingKey
             * @param data
             * @param cb
             * @param mandatory
             */
            void sendAsync(const std::string& exchange, const std::string& routingKey,
                           const std::shared_ptr<std::vector<char>>& data, const AsyncHandler& cb);

            void addSubscription(const std::string& exchange, const std::string& routingKey,
                                 AmqpCppMessageCallback&& readCb, const AsyncHandler& onComplete) {
                // Create transceiver object ...
                auto t = this->transceiver(exchange, m_instanceId, routingKey, true);
                // Register callback in transceiver
                t->onMessage(std::move(readCb));
                // Get broker assigned queue name
                std::string recvQueue = this->getRecvQueueName();
                t->startAsync(shared_from_this(), recvQueue, onComplete);
            }

            void removeSubscription(const std::string& exchange, const std::string& routingKey,
                                    const AsyncHandler& onComplete);

            /**
             * @brief activate asynchronously all transceivers: bringing them to 'ready' state.
             *  If some are ready or on the way to be active then this start has no effect.
             *
             * @param onComplete
             */
            void startAsync(const AsyncHandler& onComplete);

            /**
             * @brief close all transceivers by bringing them to the 'end' state with broker interaction.
             *
             * @param onComplete callback called at the end
             */
            void closeAsync(const AsyncHandler& onComplete);

            /**
             * @brief close and erase all transceivers asynchronously
             *
             * @param onComplete
             */
            void stopAsync(const AsyncHandler& onComplete);

            /**
             * @brief Drop and, possibly, erase all transceivers from connector
             * Drop means forcibly initialize them without communicating to the broker
             *
             * @param eraseFlag
             * @return boost::system::error_code
             */
            boost::system::error_code dropAndEraseIfShutdown(bool eraseFlag = false);


            /**
             * @brief Callback that called by AmqpSingleton object to drop all transceivers in given connector,
             * but keep tranceiver's list.
             *
             * @param message error message as a reason for such drop
             */
            void onDropConnectorCallback(const std::string& message);

            /**
             * @brief Callback that called by AmqpSingleton object to start all available transceivers
             * for this connector
             *
             */
            void onStartConnectorCallback();
        };

        // clang-format off
        using AmqpReadHashHandler = boost::function<void(const boost::system::error_code&,
                                                         const util::Hash::Pointer /*readHash*/)>;
        // clang-format on,


        //------------------------------------------------------------------------------------ AmqpClient

        /**
         * @brief Class that expose usual client API
         *
         */
        class AmqpClient : public boost::enable_shared_from_this<AmqpClient> {
           public:
            KARABO_CLASSINFO(AmqpClient, "AmqpClient", "2.0")
            KARABO_CONFIGURATION_BASE_CLASS

            static void expectedParameters(karabo::util::Schema& expected);

            AmqpClient(const karabo::util::Hash& input, const AMQP::Table& queueArgs);

            virtual ~AmqpClient();

            /**
             * Establish physical and logical connection with external RabbitMQ broker (server)
             */
            boost::system::error_code connect();

            /**
             * Check if the client is connected to the broker
             * @return true if client is connected to the broker
             */
            bool isConnected() const;

            /**
             * Disconnect itself from the broker by sending special message via synchronous write.
             */
            boost::system::error_code disconnect();

            // Non-blocking, asynchronous API

            /**
             * Disconnect from a broker (server) by sending special message via asynchronous write.
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            void disconnectAsync(const AsyncHandler& onComplete = [](const boost::system::error_code&) {});

            // /**
            //  * Force disconnect. It is not a clean disconnect sequence.<BR>
            //  * A <bi>will</bi> will be sent
            //  */
            // void disconnectForced();

            void registerConsumerHandler(const AmqpReadHashHandler& slotFunc) {
                m_onRead = slotFunc;
            }

            /**
             * Synchronous subscription by linking broker queue to the exchange
             * by means of binding key.  This call is blocked until subscription
             * process is completed and the error code is returned.
             * @param exchange used as message source
             * @param handlerKey the key associated with registered read handler
             * @return boost::system::error_code
             */
            boost::system::error_code subscribe(const std::string& exchange, const std::string& handlerKey);

            /**
             * Asynchronous subscription by linking broker queue to the exchange
             * by means of binding key. When subscription process is complete,
             * the onComplete callback is called
             * @param exchange   name of exchange point
             * @param routingKey key
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            void subscribeAsync(const std::string& exchange, const std::string& routingKey,
                                const AsyncHandler& onComplete);

            /**
             * Un-subscribe from the exchange with binding key.  The call is blocked
             * until the un-subscription process is complete.  The resulting error code
             * is returned.
             * @param exchange
             * @param routingKey key
             * @return boost::system::error_code indicating if un-subscription is successful
             */
            boost::system::error_code unsubscribe(const std::string& exchange, const std::string& routingKey);

            /**
             * Disconnect our queue from exchange with binding key.  The call is non-blocking
             * and the callback is called when un-subscription process is completed with the
             * error code as an argument.
             * @param exchange
             * @param routingKey key
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            void unsubscribeAsync(
                  const std::string& exchange, const std::string& routingKey,
                  const AsyncHandler& onComplete = [](const boost::system::error_code&) {});


            /**
             * Check if the subscription to exchange with given binding key  is
             * already registered
             * @param exchange
             * @param routing key
             * @return true or false
             */
            bool isSubscribed(const std::string& exchange, const std::string& routingKey);

            /**
             * Synchronous publishing a message to the RabbitMQ exchange with given routing key.
             * The call is blocked until operation is completed and return code is received.
             * @param exchange
             * @param routingKey
             * @param msg as a pointer to the Hash
             * @return error code of this action
             */
            boost::system::error_code publish(const std::string& exchange, const std::string& routingKey,
                                              const karabo::util::Hash::Pointer& msg);

            /**
             * Asynchronous publishing a message (Hash) asynchronously to the exchange with routing key.
             * The call is non-blocking and the callback is called when this action is complete.
             * @param exchange
             * @param routing key
             * @param msg   to publish
             * @param onComplete
             */
            void publishAsync(const std::string& exchange, const std::string& routingKey,
                              const karabo::util::Hash::Pointer& msg, const AsyncHandler& onComplete = AsyncHandler());

            boost::system::error_code close();

            void closeAsync(const AsyncHandler& onComplete);

            void setInstanceId(const std::string& instanceId) noexcept {
                m_instanceId = instanceId;
            }

            void setDomain(const std::string& domain) noexcept {
                m_domain = domain;
            }

            std::string getBrokerUrl() const noexcept {
                return m_connector->url();
            }

           private:
            void onMessageReceived(const AMQP::Message& m, uint64_t deliveryTag, bool redelivered);

            void deserialize(const std::string& exchange, const std::string& key, const std::shared_ptr<std::vector<char>>& vec, uint64_t deliveryTag,
                                     bool redelivered);

            template <typename CompletionToken>
            void post(CompletionToken&& token) {
                m_connector->post(std::forward<CompletionToken>(token));
            }

           protected:
            std::vector<std::string> m_brokerUrls;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_binarySerializer;
            std::string m_domain;
            std::string m_instanceId;
            const std::uint32_t m_amqpRequestTimeout;
            AmqpConnector::Pointer m_connector;
            karabo::net::Strand::Pointer m_serializerStrand;
            karabo::net::Strand::Pointer m_strand;
            AmqpReadHashHandler m_onRead;
            bool m_skipFlag;
            AMQP::Table m_queueArgs;
        };
    } // namespace net
} // namespace karabo

#endif /* KARABO_NET_AMQPCLIENT_HH */
