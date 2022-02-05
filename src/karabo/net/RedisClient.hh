/*
 * File:   RedisClient.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on May 6, 2021, 12:12 PM
 */

#ifndef KARABO_NET_REDISCLIENT_HH
#define KARABO_NET_REDISCLIENT_HH

#include <boost/asio.hpp>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "karabo/io/BinarySerializer.hh"
#include "karabo/net/utils.hh"
#include "karabo/util/ClassInfo.hh"
#include "redisclient/redisasyncclient.h"

// clang-format off
#define KARABO_ERROR_CODE_SUCCESS           boost::system::errc::make_error_code(boost::system::errc::success)
#define KARABO_ERROR_CODE_CONN_REFUSED      boost::system::errc::make_error_code(boost::system::errc::connection_refused)
#define KARABO_ERROR_CODE_IO_ERROR          boost::system::errc::make_error_code(boost::system::errc::io_error)
#define KARABO_ERROR_CODE_OP_CANCELLED      boost::system::errc::make_error_code(boost::system::errc::operation_canceled)
#define KARABO_ERROR_CODE_NOT_CONNECTED     boost::system::errc::make_error_code(boost::system::errc::not_connected)
#define KARABO_ERROR_CODE_ALREADY_CONNECTED boost::system::errc::make_error_code(boost::system::errc::already_connected)
#define KARABO_ERROR_CODE_TIMED_OUT         boost::system::errc::make_error_code(boost::system::errc::timed_out)
#define KARABO_ERROR_CODE_STREAM_TIMEOUT    boost::system::errc::make_error_code(boost::system::errc::stream_timeout)
#define KARABO_ERROR_CODE_RESOURCE_BUSY     boost::system::errc::make_error_code(boost::system::errc::device_or_resource_busy)
// clang-format on


namespace karabo {
    namespace net {


        using ReadHashHandler = std::function<void(const boost::system::error_code, const std::string& /*topic*/,
                                                   const util::Hash::Pointer /*readHash*/)>;
        using RedisTopicSubOptions = std::vector<std::tuple<std::string, ReadHashHandler> >;


        class RedisClient : public boost::enable_shared_from_this<RedisClient> {
           public:
            KARABO_CLASSINFO(RedisClient, "RedisClient", "2.0")

            static void expectedParameters(karabo::util::Schema& expected);

            RedisClient(const karabo::util::Hash& input);

            virtual ~RedisClient();

            /**
             * Establish physical and logical connection with external Redis server
             */
            virtual boost::system::error_code connect();

            /**
             * Establish physical and logical connection with external Redis server
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            virtual void connectAsync(const AsyncHandler& onComplete = [](const boost::system::error_code&) {});

            /**
             * Check if the client is connected to the broker
             * @return true if client is connected to the broker
             */
            virtual bool isConnected() const;

            /**
             * Disconnect itself from the broker (server) by sending special message via synchronous write.
             */
            virtual boost::system::error_code disconnect();

            // Non-blocking, asynchronous API

            /**
             * Disconnect from a broker (server) by sending special message via asynchronous write.
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            virtual void disconnectAsync(const AsyncHandler& onComplete = [](const boost::system::error_code&) {});

            /**
             * Force disconnect. It is not a clean disconnect sequence.<BR>
             */
            virtual void disconnectForced();

            /**
             * Synchronous single topic subscription == Redis channel subscription
             * The topic name is case-sensitive and may contain "/" to represent hierarchical multi-level
             * name.  The topic name can be exact (specific) topic name or contain single-level ("+") or
             * multi-level ("#") wildcard symbols.  Normally the call is blocking: "real" subscription
             * request message is sending to the broker and waiting for acknowledgment. Locally we register
             * the 'slotFunc' callback that will be called for processing when the message is received
             * due to this subscription. Only one callback per topic is allowed. If the topic is a subset
             * of already subscribed topics with wildcards it is "overlapped subscription" and
             * may result in receiving the same message twice and the callback registered with specific
             * topic subscription is called twice!  If the topic name is already subscribed
             * then the new callback (slotFunc) will replace locally the existing one so no communication
             * with the broker is needed. Therefore no replacement of 'subopts' is possible!
             *
             * @param topic to be subscribed
             * @param read handler called when the message associated with this topic arrived
             * @return boost::system::error_code
             */
            boost::system::error_code subscribe(const std::string& topic, const ReadHashHandler& slotFunc);

            /**
             * Asynchronous single topic subscription == Redis channel subscription.
             * This is an asynchronous version of synchronous single topic call. The call is non-blocking
             * and onComplete handler will be called after receiving broker acknowledgment.
             * @param topic to be subscribed
             * @param slotFunction - handler called when a message associated with topic arrived
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            void subscribeAsync(const std::string& topic, const ReadHashHandler& slotFunc,
                                const AsyncHandler& onComplete);

            /**
             * Synchronous multiple topics subscription.
             * Subscriptions are represented as vector of tuples of topic name, quality-of-service and
             * read callback.  The topic names can be with or without wildcards. It allows to do single call
             * to subscribe to many topics. No special handling of overlapped subscriptions. No special
             * rules about order of topics in the list.
             * @param params  multiple tuples of 'topic', 'qos', 'slotFunction' to subscribe
             * @return boost::system::error_code
             */
            virtual boost::system::error_code subscribe(const RedisTopicSubOptions& params);

            /**
             * Asynchronous multiple topics subscription.
             * This is an asynchronous version of synchronous multiple topics call. The call is non-blocking
             * and onComplete handler will be called after receiving broker acknowledgment.
             * @param params   multiple (<64) tuples of 'topic', 'qos', 'slotName', 'slotFunction' to subscribe
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            virtual void subscribeAsync(const RedisTopicSubOptions& params, const AsyncHandler& onComplete);

            /**
             * Request Redis broker to un-subscribe the topic.
             * If the topic not known then nothing is changed.
             * If un-subscribing is successful then the registered read callback associated
             * with the topic is removed, otherwise nothing is changed  .
             * @param topic to un-subscribe
             * @return boost::system::error_code indicating if un-subscription is successful
             */
            virtual boost::system::error_code unsubscribe(const std::string& topic);

            /**
             * Request broker to un-subscribe topic.
             * @param topic to un-subscribe
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            virtual void unsubscribeAsync(
                  const std::string& topic, const AsyncHandler& onComplete = [](const boost::system::error_code&) {});

            /**
             * Request broker to un-subscribe multiple topics
             * @param multiple topic-handlerName tuples to un-subscribe
             * @return boost::system::error_code indicating if un-subscription is successful (all or none)
             */
            virtual boost::system::error_code unsubscribe(const std::vector<std::string>& topics);

            /**
             * Request broker to un-subscribe many topics
             * @param multiple topics to un-subscribe
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            virtual void unsubscribeAsync(
                  const std::vector<std::string>& topics,
                  const AsyncHandler& onComplete = [](const boost::system::error_code&) {});

            /**
             * Un-subscribe from all subscriptions made by this client upto now (blocking call)
             * @return boost::system::error_code indicating if un-subscribing was successful (all or none)
             */
            virtual boost::system::error_code unsubscribeAll();

            /**
             * Un-subscribe from all subscriptions made by this client upto now (non-blocking call)
             * @param onUnSubAck acknowledgment callback will be called at the end
             * @param callback after network completion (send operation was successful)
             */
            virtual void unsubscribeAllAsync(const AsyncHandler& onComplete = [](const boost::system::error_code&) {});

            /**
             * Check if the topic is subscribed already.
             * @param topic
             * @return true or false
             */
            virtual bool isSubscribed(const std::string& topic);

            boost::system::error_code publish(const std::string& topic, const karabo::util::Hash::Pointer& msg);

            /**
             * Publish a message (Hash) asynchronously on the topic with given options
             * @param topic to publish
             * @param msg   to publish
             * @param pubopts Example: PubQos::ExactlyOnce | PubRetain::yes
             * @param onComplete
             */
            void publishAsync(const std::string& topic, const karabo::util::Hash::Pointer& msg,
                              const AsyncHandler& onComplete = AsyncHandler());

            virtual const std::string& getBrokerUrl() const;

            /**
             * Retrieve all topics we are subscribed to so far. Wildcards
             * subscriptions are book-kept as they were defined.
             * @return list of topics including wildcards
             */
            virtual std::vector<std::string> getSubscriptions();

            template <typename CompletionToken>
            void post(CompletionToken&& token) {
                m_ios->post(token);
            }

           private:
            /**
             * Disptch a functor on REDIS internal event loop
             * @param token == functor
             */
            template <typename CompletionToken>
            void dispatch(CompletionToken&& token) {
                m_ios->dispatch(token);
            }

            void run();

            void createClientForUrl(const std::string& url, const AsyncHandler& onConnect);

            void resolveHandler(const boost::system::error_code& e, boost::asio::ip::tcp::resolver::iterator it,
                                const AsyncHandler& onConnect);

           protected:
            boost::shared_ptr<boost::asio::io_context> m_ios;
            boost::shared_ptr<boost::thread> m_thread;
            boost::shared_ptr<redisclient::RedisAsyncClient> m_producer;
            boost::shared_ptr<redisclient::RedisAsyncClient> m_consumer;
            boost::asio::ip::tcp::resolver m_resolver;
            std::size_t m_brokerIndex;
            std::vector<std::string> m_brokerUrls;
            // Mutex used to avoid concurrent calls of connectAsync
            std::mutex m_connectionMutex;
            // Mutex used to avoid concurrent calls of disconnectAsync
            std::mutex m_disconnectionMutex;
            // Mutex used to avoid concurrent calls of subscribeAsync
            std::mutex m_subscribeMutex;
            std::unordered_map<std::string, std::tuple<bool, redisclient::RedisAsyncClient::Handle> >
                  m_subscriptionsMap;
            std::mutex m_subscriptionsMutex;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_binarySerializer;
            std::uint32_t m_requestTimeout;
            bool m_skipFlag;
        };


    } // namespace net
} // namespace karabo

#endif /* KARABO_NET_REDISCLIENT_HH */
