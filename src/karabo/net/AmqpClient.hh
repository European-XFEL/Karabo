/* 
 * File:   AmqpClient.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on May 18, 2021, 1:18 PM
 */

#ifndef KARABO_NET_AMQPCLIENT_HH
#define	KARABO_NET_AMQPCLIENT_HH

#include <boost/asio.hpp>
#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>
#include "karabo/io/BinarySerializer.hh"
#include "karabo/util/Configurator.hh"      // KARABO_CONFIGURATION_BASE_CLASS
#include "utils.hh"
#include "EventLoop.hh"                         // AsyncHandler
#include "Strand.hh"


#define KARABO_ERROR_CODE_SUCCESS           boost::system::errc::make_error_code(boost::system::errc::success)
#define KARABO_ERROR_CODE_IO_ERROR          boost::system::errc::make_error_code(boost::system::errc::io_error)
#define KARABO_ERROR_CODE_CONNECT_REFUSED   boost::system::errc::make_error_code(boost::system::errc::connection_refused)
#define KARABO_ERROR_CODE_OP_CANCELLED      boost::system::errc::make_error_code(boost::system::errc::operation_canceled)
#define KARABO_ERROR_CODE_NOT_CONNECTED     boost::system::errc::make_error_code(boost::system::errc::not_connected)
#define KARABO_ERROR_CODE_ALREADY_CONNECTED boost::system::errc::make_error_code(boost::system::errc::already_connected)
#define KARABO_ERROR_CODE_TIMED_OUT         boost::system::errc::make_error_code(boost::system::errc::timed_out)
#define KARABO_ERROR_CODE_STREAM_TIMEOUT    boost::system::errc::make_error_code(boost::system::errc::stream_timeout)
#define KARABO_ERROR_CODE_RESOURCE_BUSY     boost::system::errc::make_error_code(boost::system::errc::device_or_resource_busy)


namespace karabo {
    namespace net {


        /**
         *  Custom AMQP TCP handler
         */
        class CustomTcpHandler : public AMQP::LibBoostAsioHandler
        {
            std::shared_ptr<std::promise<bool> > m_connected;
            std::string m_status;

        public:

            void setPromise(std::shared_ptr<std::promise<bool> > p) {
                m_connected = p;
                m_status = "init";
            }

            const std::string& getStatus() const { return m_status; }

        private:
            /**
             *  Method that is called when a connection error occurs
             *  @param  connection
             *  @param  message
             */
            virtual void onError(AMQP::TcpConnection *connection, const char *message) override {
                m_status = message;
                m_connected->set_value(false);
            }

            /**
             *  Method that is called when the TCP connection ends up in a connected state
             *  @param  connection  The TCP connection
             */
            virtual void onConnected(AMQP::TcpConnection *connection) override {
                m_status = "connected";
                m_connected->set_value(true);
            }

            /**
             *  Method that is called when the TCP connection ends up in a ready
             *  @param  connection  The TCP connection
             */
            virtual void onReady(AMQP::TcpConnection *connection) override {
                m_status = "ready";
            }

            /**
             *  Method that is called when the TCP connection is closed
             *  @param  connection  The TCP connection
             */
            virtual void onClosed(AMQP::TcpConnection *connection) override {
                m_status = "closed";
            }

            /**
             *  Method that is called when the TCP connection is detached
             *  @param  connection  The TCP connection
             */
            virtual void onDetached(AMQP::TcpConnection *connection) override {
                m_status = "detached";
            }

        public:
            /**
             *  Constructor
             *  @param  event loop (Boost ASIO io_context reference)
             */
            CustomTcpHandler(boost::asio::io_context& loop) : AMQP::LibBoostAsioHandler(loop) {}

            /**
             *  Destructor
             */
            virtual ~CustomTcpHandler() = default;
        };


        // Helper class (singleton)
        class TcpConnector
        {
        private:

            static std::weak_ptr<TcpConnector> m_weakConnector;
            std::shared_ptr<std::thread> m_thread;
            std::shared_ptr<boost::asio::io_context> m_loop;
            CustomTcpHandler m_handler;
            std::shared_ptr<AMQP::TcpConnection> m_connection;
            std::string m_url;

            void run();

            TcpConnector() = delete;
            TcpConnector(const TcpConnector&) = delete;

        public:

            typedef std::shared_ptr<TcpConnector> Pointer;

            TcpConnector(const std::vector<std::string>& urls);

            virtual ~TcpConnector();

            static TcpConnector::Pointer create(const std::vector<std::string>& urls);

            std::shared_ptr<AMQP::TcpConnection> native() { return m_connection; }

            const std::string& url() const { return m_url; }

            std::string getStatus() const { return m_handler.getStatus(); }

            const std::shared_ptr<boost::asio::io_context>& getContext() const { return m_loop; }
        };


        using ReadHashHandler = std::function<void(const boost::system::error_code,
                                                   const std::string& /*exchange*/,
                                                   const std::string& /*routing key*/,
                                                   const util::Hash::Pointer /*readHash*/)>;


        class AmqpClient : public boost::enable_shared_from_this<AmqpClient> {

        public:

            KARABO_CLASSINFO(AmqpClient, "AmqpClient", "2.0")
            KARABO_CONFIGURATION_BASE_CLASS

            static void expectedParameters(karabo::util::Schema& expected);

            AmqpClient(const karabo::util::Hash& input);

            virtual ~AmqpClient();

            /**
             * Establish physical and logical connection with external RabbitMQ broker (server)
             */
	    virtual boost::system::error_code connect();

            /**
             * Establish physical and logical connection with external RabbitMQ broker (server)
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            virtual void connectAsync(const AsyncHandler& onComplete = [](const boost::system::error_code&){});

            /**
             * Check if the client is connected to the broker
             * @return true if client is connected to the broker
             */
            virtual bool isConnected() const;

            /**
             * Disconnect itself from the broker by sending special message via synchronous write.
             */
            virtual boost::system::error_code disconnect();

            // Non-blocking, asynchronous API

            /**
             * Disconnect from a broker (server) by sending special message via asynchronous write.
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            virtual void disconnectAsync(const AsyncHandler& onComplete = [](const boost::system::error_code&){});

            /**
             * Force disconnect. It is not a clean disconnect sequence.<BR>
             * A <bi>will</bi> will be sent
             */
            virtual void disconnectForced();

            void registerConsumerHandler(const ReadHashHandler& slotFunc) {
                m_onRead = slotFunc;
            }

            /**
             * Synchronous subscription by linking broker queue to the exchange
             * by means of binding key.  This call is blocked until subscription
             * process is completed and the error code is returned.
             * @param exchange used as message source
             * @param binding key used to select specific messages from exchange
             * @return boost::system::error_code 
             */
            boost::system::error_code
            subscribe(const std::string& exchange, const std::string& bindingKey);

            /**
             * Asynchronous subscription by linking broker queue to the exchange
             * by means of binding key. When subscription process is complete,
             * the onComplete callback is called
             * @param exchange
             * @param binding key
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            void
            subscribeAsync(const std::string& exchange, const std::string& routingKey, const AsyncHandler& onComplete);

            /**
             * Un-subscribe from the exchange with binding key.  The call is blocked
             * until the un-subscription process is complete.  The resulting error code
             * is returned.
             * @param exchange
             * @param routing key
             * @return boost::system::error_code indicating if un-subscription is successful
             */
            virtual boost::system::error_code
            unsubscribe(const std::string& exchange, const std::string& routingKey);

            /**
             * Disconnect our queue from exchange with binding key.  The call is non-blocking
             * and the callback is called when un-subscription process is completed with the
             * error code as an argument.
             * @param exchange
             * @param routing key
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            virtual void
            unsubscribeAsync(const std::string& exchange, const std::string& routingKey,
                             const AsyncHandler& onComplete = [](const boost::system::error_code&){});


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
            boost::system::error_code
            publish(const std::string& exchange, const std::string& routingKey, const karabo::util::Hash::Pointer& msg);

            /**
             * Asynchronous publishing a message (Hash) asynchronously to the exchange with routing key.
             * The call is non-blocking and the callback is called when this action is complete.
             * @param exchange
             * @param routing key
             * @param msg   to publish
             * @param onComplete
             */
            void
            publishAsync(const std::string& exchange, const std::string& routingKey,
                         const karabo::util::Hash::Pointer& msg,
                         const AsyncHandler& onComplete = AsyncHandler());

            boost::system::error_code close();

            void closeAsync(const AsyncHandler& onComplete);

            void setInstanceId(const std::string& instanceId) { m_instanceId = instanceId; }

            void setDomain(const std::string& domain) { m_domain = domain; }

            virtual const std::string& getBrokerUrl() const;

        private:

            template<typename CompletionToken>
            void dispatch(CompletionToken&& token) {
                m_connector->getContext()->dispatch(std::forward<CompletionToken>(token));  // post on EventLoop
            }

            std::shared_ptr<AMQP::TcpConnection>
            createClientForUrls(const std::vector<std::string>& urls);

            void createChannel(std::shared_ptr<AMQP::TcpConnection> connection, const AsyncHandler& onConnect);

            boost::system::error_code
            publishImpl(const std::string& exchange,
                        const std::string& routingkey,
                        const karabo::util::Hash::Pointer& msg);

            void onCloseError(const char* message, const AsyncHandler& onComplete);

            void onCloseSuccess(const AsyncHandler& onComplete);

            void onConnectError(const char* message, const AsyncHandler& onConnect);

            void onConnectReady(const AsyncHandler& onConnect);

            void onDeclareQueueError(const char* message, const AsyncHandler& onConnect);

            void onDeclareQueueSuccess(const std::string& name, uint32_t messageCount,
                                       uint32_t consumerCount, const AsyncHandler& onConnect);

            void onConsumerConnectError(const char* message, const AsyncHandler& onConnect);

            void onMessageReceived(const AMQP::Message &m, uint64_t deliveryTag, bool /*redelivered*/);

            void onDeclareExchangeError(const char* message,
                                        const std::string& exchange,
                                        const AsyncHandler& onComplete);

            void onDeclareExchangeSuccess(const std::string& exchange,
                                          const std::string& bindingKey,
                                          const AsyncHandler& onComplete);

            void onBindQueueError(const char* message,
                                  const std::string& exchange,
                                  const std::string& bindingKey,
                                  const AsyncHandler& onComplete);

            void onBindQueueSuccess(const std::string& exchange,
                                    const std::string& bindingKey,
                                    const AsyncHandler& onComplete);
            void onUnbindQueueError(const char* message,
                                    const std::string& exchange,
                                    const std::string& routingKey,
                                    const AsyncHandler& onComplete);

            void onUnbindQueueSuccess(const AsyncHandler& onComplete);

        protected:

            Strand::Pointer m_strand;
            std::vector<std::string> m_brokerUrls;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_binarySerializer;
            std::string m_domain;
            std::string m_instanceId;
            int m_amqpRequestTimeout;
            // set of <exchange> + <bindingKey>
            std::unordered_set<std::string> m_subscriptions;
            std::mutex m_subscriptionsMutex;

            TcpConnector::Pointer m_connector;
            std::shared_ptr<AMQP::TcpChannel> m_channel;
            std::shared_ptr<AMQP::Reliable<> > m_reliable;
            std::string m_queue;
            std::string m_consumerTag;
            ReadHashHandler m_onRead;
            static std::unordered_set<std::string> m_registeredExchanges;
        };
    }
}

#endif	/* KARABO_NET_AMQPCLIENT_HH */

