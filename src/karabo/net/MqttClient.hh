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
 * File:   MqttClient.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on January 17, 2018, 11:57 AM
 */

#ifndef KARABO_NET_MQTTCLIENT_HH
#define KARABO_NET_MQTTCLIENT_HH

#include <algorithm>
#include <boost/asio.hpp>
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         //  streaming operators etc.
#include <tuple>
#include <vector>

#include "karabo/util/Configurator.hh" // KARABO_CONFIGURATION_BASE_CLASS
#include "utils.hh"

#define KARABO_ERROR_CODE_SUCCESS boost::system::errc::make_error_code(boost::system::errc::success)
#define KARABO_ERROR_CODE_IO_ERROR boost::system::errc::make_error_code(boost::system::errc::io_error)
#define KARABO_ERROR_CODE_OP_CANCELLED boost::system::errc::make_error_code(boost::system::errc::operation_canceled)
#define KARABO_ERROR_CODE_NOT_CONNECTED boost::system::errc::make_error_code(boost::system::errc::not_connected)
#define KARABO_ERROR_CODE_ALREADY_CONNECTED boost::system::errc::make_error_code(boost::system::errc::already_connected)
#define KARABO_ERROR_CODE_TIMED_OUT boost::system::errc::make_error_code(boost::system::errc::timed_out)
#define KARABO_ERROR_CODE_STREAM_TIMEOUT boost::system::errc::make_error_code(boost::system::errc::stream_timeout)
#define KARABO_ERROR_CODE_RESOURCE_BUSY \
    boost::system::errc::make_error_code(boost::system::errc::device_or_resource_busy)

#define KARABO_ASSERT(expr)                    \
    {                                          \
        std::ostringstream oss;                \
        oss << __FILE__ << ':' << __LINE__;    \
        assert((expr) && (oss.str().c_str())); \
    }


namespace karabo {
    namespace net {
        namespace mqtttools {

            bool topicMatches(const std::string& str, const std::string& topic);

            /**
             * Predicate to check that the topic contains wildcard characters.
             * @param topic
             * @return true or false
             */
            bool topicHasWildcard(const std::string& topic);
        } // namespace mqtttools


        using ReadHashHandler = std::function<void(const boost::system::error_code, const std::string& /*topic*/,
                                                   const util::Hash::Pointer /*readHash*/)>;

        //********    PUBLISH Options    *********
        // See: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901101

        enum class PubQos : std::uint8_t {
            AtMostOnce = 0b00000000,  // qos = 0
            AtLeastOnce = 0b00000010, // qos = 1
            ExactlyOnce = 0b00000100, // qos = 2
        };

        enum class PubRetain : std::uint8_t {
            no = 0b00000000,
            yes = 0b00000001, // retain flag
        };

        enum class PubDup : std::uint8_t {
            no = 0b00000000,
            yes = 0b00001000, // Duplicate (re-transmission)
        };

        struct PubOpts final {
            constexpr PubOpts(void) = default;
            ~PubOpts(void) = default;
            constexpr PubOpts(PubOpts&&) = default;
            constexpr PubOpts(PubOpts const&) = default;
            constexpr PubOpts& operator=(PubOpts&&) = default;
            constexpr PubOpts& operator=(PubOpts const&) = default;

            explicit constexpr PubOpts(std::uint8_t value) : m_data(value) {}

            constexpr PubOpts(PubQos value) : m_data(static_cast<std::uint8_t>(value)) {}
            constexpr PubOpts(PubRetain value) : m_data(static_cast<std::uint8_t>(value)) {}
            constexpr PubOpts(PubDup value) : m_data(static_cast<std::uint8_t>(value)) {}

            constexpr PubOpts operator|(const PubOpts& rhs) const {
                return PubOpts(m_data | rhs.m_data);
            }
            constexpr PubOpts operator|(const PubQos& rhs) const {
                return *this | PubOpts(rhs);
            }
            constexpr PubOpts operator|(const PubRetain& rhs) const {
                return *this | PubOpts(rhs);
            }
            constexpr PubOpts operator|(const PubDup& rhs) const {
                return *this | PubOpts(rhs);
            }

            constexpr PubOpts operator|=(const PubOpts& rhs) {
                return (*this = (*this | rhs));
            }
            constexpr PubOpts operator|=(const PubQos& rhs) {
                return (*this = (*this | rhs));
            }
            constexpr PubOpts operator|=(const PubRetain& rhs) {
                return (*this = (*this | rhs));
            }
            constexpr PubOpts operator|=(const PubDup& rhs) {
                return (*this = (*this | rhs));
            }

            constexpr PubQos getPubQos() const {
                return static_cast<PubQos>(m_data & 0b00000110);
            }

            constexpr PubRetain getPubRetain() const {
                return static_cast<PubRetain>(m_data & 0b00000001);
            }

            constexpr PubDup getPubDup() const {
                return static_cast<PubDup>(m_data & 0b00001000);
            }

            explicit constexpr operator std::uint8_t() const {
                return m_data;
            }

           private:
            std::uint8_t m_data = 0;
        };


        constexpr PubOpts operator|(PubQos lhs, PubRetain rhs) {
            return PubOpts(lhs) | rhs;
        }
        constexpr PubOpts operator|(PubQos lhs, PubDup rhs) {
            return PubOpts(lhs) | rhs;
        }

        constexpr PubOpts operator|(PubRetain lhs, PubQos rhs) {
            return PubOpts(lhs) | rhs;
        }
        constexpr PubOpts operator|(PubRetain lhs, PubDup rhs) {
            return PubOpts(lhs) | rhs;
        }

        constexpr PubOpts operator|(PubDup lhs, PubRetain rhs) {
            return PubOpts(lhs) | rhs;
        }
        constexpr PubOpts operator|(PubDup lhs, PubQos rhs) {
            return PubOpts(lhs) | rhs;
        }

        constexpr char const* pubQosToString(PubQos v) {
            return (v == PubQos::AtMostOnce ? "at_most_once"
                                            : (v == PubQos::AtLeastOnce
                                                     ? "at_least_once"
                                                     : (v == PubQos::ExactlyOnce ? "exactly_once" : "invalid_PubQos")));
        }

        template <typename Stream>
        Stream& operator<<(Stream& os, PubQos val) {
            os << pubQosToString(val);
            return os;
        }

        constexpr char const* pubRetainToString(PubRetain v) {
            return (v == PubRetain::no ? "no" : (v == PubRetain::yes ? "yes" : "invalid_PubRetain"));
        }

        template <typename Stream>
        Stream& operator<<(Stream& os, PubRetain val) {
            os << pubRetainToString(val);
            return os;
        }

        constexpr char const* pubDupToString(PubDup v) {
            return (v == PubDup::no ? "no" : (v == PubDup::yes ? "yes" : "invalid_PubDup"));
        }

        template <typename Stream>
        Stream& operator<<(Stream& os, PubDup val) {
            os << pubDupToString(val);
            return os;
        }

        template <typename Stream>
        Stream& operator<<(Stream& os, PubOpts val) {
            os << "{qos=" << val.getPubQos() << ", retain=" << val.getPubRetain() << ", dup=" << val.getPubDup() << "}";
            return os;
        }


        //********    SUBSCRIBE Options    *********
        // See: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901169

        // SUBSCRIBE Option: Quality Of Service
        enum class SubQos : std::uint8_t {
            AtMostOnce = 0b00000000,  // qos = 0
            AtLeastOnce = 0b00000001, // qos = 1
            ExactlyOnce = 0b00000010, // qos = 2
        };

        // SUBSCRIBE Option: No Local
        enum class SubNoLocal : std::uint8_t {
            no = 0b00000000,
            yes = 0b00000100, // No Local option
        };

        // SUBSCRIBE Option: Retain As Published
        enum class SubRetainAsPublished : std::uint8_t {
            no = 0b00000000,
            yes = 0b00001000, // Retain As Published option
        };

        // SUBSCRIBE Option: Retain Handling
        enum class SubRetainHandling : std::uint8_t {
            send = 0b00000000,                    // send
            sendOnlyNewSubscription = 0b00010000, // retain handling
            notSend = 0b00100000                  // retain handling
        };

        struct SubOpts final {
            constexpr SubOpts(void) = delete;
            ~SubOpts(void) = default;
            constexpr SubOpts(SubOpts&&) = default;
            constexpr SubOpts(SubOpts const&) = default;
            constexpr SubOpts& operator=(SubOpts&&) = default;
            constexpr SubOpts& operator=(SubOpts const&) = default;

            explicit constexpr SubOpts(std::uint8_t value) : m_data(value) {}

            constexpr SubOpts(SubQos value) : m_data(static_cast<std::uint8_t>(value)) {}
            constexpr SubOpts(SubNoLocal value) : m_data(static_cast<std::uint8_t>(value)) {}
            constexpr SubOpts(SubRetainAsPublished value) : m_data(static_cast<std::uint8_t>(value)) {}
            constexpr SubOpts(SubRetainHandling value) : m_data(static_cast<std::uint8_t>(value)) {}

            constexpr SubOpts operator|(SubOpts rhs) const {
                return SubOpts(m_data | rhs.m_data);
            }
            constexpr SubOpts operator|(SubQos rhs) const {
                return *this | SubOpts(rhs);
            }
            constexpr SubOpts operator|(SubNoLocal rhs) const {
                return *this | SubOpts(rhs);
            }
            constexpr SubOpts operator|(SubRetainAsPublished rhs) const {
                return *this | SubOpts(rhs);
            }
            constexpr SubOpts operator|(SubRetainHandling rhs) const {
                return *this | SubOpts(rhs);
            }

            constexpr SubOpts operator|=(const SubOpts& rhs) {
                return (*this = (*this | rhs));
            }
            constexpr SubOpts operator|=(const SubQos& rhs) {
                return (*this = (*this | rhs));
            }
            constexpr SubOpts operator|=(const SubNoLocal& rhs) {
                return (*this = (*this | rhs));
            }
            constexpr SubOpts operator|=(const SubRetainAsPublished& rhs) {
                return (*this = (*this | rhs));
            }
            constexpr SubOpts operator|=(const SubRetainHandling& rhs) {
                return (*this = (*this | rhs));
            }

            constexpr SubQos getSubQos() const {
                return static_cast<SubQos>(m_data & 0b00000011);
            }

            constexpr SubNoLocal getSubNoLocal() const {
                return static_cast<SubNoLocal>(m_data & 0b00000100);
            }

            constexpr SubRetainAsPublished getSubRetainAsPublished() const {
                return static_cast<SubRetainAsPublished>(m_data & 0b00001000);
            }

            constexpr SubRetainHandling getSubRetainHandling() const {
                return static_cast<SubRetainHandling>(m_data & 0b00110000);
            }

            explicit constexpr operator std::uint8_t() const {
                return m_data;
            }

           private:
            std::uint8_t m_data;
        };


        constexpr SubOpts operator|(SubQos lhs, SubNoLocal rhs) {
            return SubOpts(lhs) | rhs;
        }
        constexpr SubOpts operator|(SubQos lhs, SubRetainAsPublished rhs) {
            return SubOpts(lhs) | rhs;
        }
        constexpr SubOpts operator|(SubQos lhs, SubRetainHandling rhs) {
            return SubOpts(lhs) | rhs;
        }

        constexpr SubOpts operator|(SubNoLocal lhs, SubQos rhs) {
            return SubOpts(lhs) | rhs;
        }
        constexpr SubOpts operator|(SubNoLocal lhs, SubRetainAsPublished rhs) {
            return SubOpts(lhs) | rhs;
        }
        constexpr SubOpts operator|(SubNoLocal lhs, SubRetainHandling rhs) {
            return SubOpts(lhs) | rhs;
        }

        constexpr SubOpts operator|(SubRetainAsPublished lhs, SubQos rhs) {
            return SubOpts(lhs) | rhs;
        }
        constexpr SubOpts operator|(SubRetainAsPublished lhs, SubNoLocal rhs) {
            return SubOpts(lhs) | rhs;
        }
        constexpr SubOpts operator|(SubRetainAsPublished lhs, SubRetainHandling rhs) {
            return SubOpts(lhs) | rhs;
        }

        constexpr SubOpts operator|(SubRetainHandling lhs, SubQos rhs) {
            return SubOpts(lhs) | rhs;
        }
        constexpr SubOpts operator|(SubRetainHandling lhs, SubNoLocal rhs) {
            return SubOpts(lhs) | rhs;
        }
        constexpr SubOpts operator|(SubRetainHandling lhs, SubRetainAsPublished rhs) {
            return SubOpts(lhs) | rhs;
        }

        constexpr char const* subRetainToString(SubRetainHandling v) {
            return (v == SubRetainHandling::send
                          ? "send"
                          : (v == SubRetainHandling::sendOnlyNewSubscription
                                   ? "send_only_new_subscription"
                                   : (v == SubRetainHandling::notSend ? "not_send" : "invalid_SubRetainHandling")));
        }

        template <typename Stream>
        Stream& operator<<(Stream& os, SubRetainHandling val) {
            os << subRetainToString(val);
            return os;
        }

        constexpr char const* subNoLocalToString(SubNoLocal v) {
            return (v == SubNoLocal::no ? "no" : (v == SubNoLocal::yes ? "yes" : "invalid_SubNoLocal"));
        }

        template <typename Stream>
        Stream& operator<<(Stream& os, SubNoLocal val) {
            os << subNoLocalToString(val);
            return os;
        }

        constexpr char const* subRetainAsPublishedToString(SubRetainAsPublished v) {
            return (v == SubRetainAsPublished::no
                          ? "no"
                          : (v == SubRetainAsPublished::yes ? "yes" : "invalid_SubRetainAsPublished"));
        }

        template <typename Stream>
        Stream& operator<<(Stream& os, SubRetainAsPublished val) {
            os << subRetainAsPublishedToString(val);
            return os;
        }

        constexpr char const* subQosToString(SubQos v) {
            return (v == SubQos::AtMostOnce ? "at_most_once"
                                            : (v == SubQos::AtLeastOnce
                                                     ? "at_least_once"
                                                     : (v == SubQos::ExactlyOnce ? "exactly_once" : "invalid_SubQos")));
        }

        template <typename Stream>
        Stream& operator<<(Stream& os, SubQos val) {
            os << subQosToString(val);
            return os;
        }

        template <typename Stream>
        Stream& operator<<(Stream& os, SubOpts val) {
            os << "{qos=" << val.getSubQos() << ", nl=" << val.getSubNoLocal()
               << ", rap=" << val.getSubRetainAsPublished() << ", rh=" << val.getSubRetainHandling() << "}";
            return os;
        }


        using TopicSubOptions = std::vector<std::tuple<std::string, SubOpts, ReadHashHandler>>;

        /**
         * @class MqClient
         * @brief This class implements a MQTT client-to-broker (c2b) messaging interface for Karabo
         *
         * This class implements a MQ client-to-broker (p2p) messaging interface for Karabo.
         * The client side can be both producer and consumer simultaneously: it can get messages from
         * other client via broker (minimum 2-hops communication) or send messages to other clients.
         * We are trying to hide all implementation details and 'mqtt_cpp' API in .cc file
         */
        class MqttClient : public boost::enable_shared_from_this<MqttClient> {
           public:
            KARABO_CLASSINFO(MqttClient, "MqttClient", "2.0")
            KARABO_CONFIGURATION_BASE_CLASS

            static void expectedParameters(karabo::util::Schema& expected);

            MqttClient(const karabo::util::Hash& input);

            virtual ~MqttClient();

            /**
             * Establish physical and logical connection with external MQTT broker (server)
             */
            virtual boost::system::error_code connect() = 0;

            /**
             * Establish physical and logical connection with external MQTT broker (server)
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            virtual void connectAsync(const AsyncHandler& onComplete = [](const boost::system::error_code&) {}) = 0;

            /**
             * Check if the client is connected to the broker
             * @return true if client is connected to the broker
             */
            virtual bool isConnected() const = 0;

            /**
             * Disconnect itself from the broker by sending special message via synchronous write.
             */
            virtual boost::system::error_code disconnect() = 0;

            // Non-blocking, asynchronous API

            /**
             * Disconnect from a broker (server) by sending special message via asynchronous write.
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            virtual void disconnectAsync(const AsyncHandler& onComplete = [](const boost::system::error_code&) {}) = 0;

            /**
             * Force disconnect. It is not a clean disconnect sequence.<BR>
             * A <bi>will</bi> will be sent
             */
            virtual void disconnectForced() = 0;

            /**
             * Synchronous single topic subscription.
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
             * @param quality of service for such subscription
             * @param read handler called when the message associated with this topic arrived
             * @return boost::system::error_code
             */
            boost::system::error_code subscribe(const std::string& topic, SubOpts subopts,
                                                const ReadHashHandler& slotFunc) {
                return subscribe(topic, std::uint8_t(subopts), slotFunc);
            }

            /**
             * Asynchronous single topic subscription.
             * This is an asynchronous version of synchronous single topic call. The call is non-blocking
             * and onComplete handler will be called after receiving broker acknowledgment.
             * @param topic to be subscribed
             * @param quality of service for such subscription
             * @param slotFunction - handler called when a message associated with topic arrived
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            void subscribeAsync(const std::string& topic, SubOpts subopts, const ReadHashHandler& slotFunc,
                                const AsyncHandler& onComplete) {
                subscribeAsync(topic, std::uint8_t(subopts), slotFunc, onComplete);
            }

            /**
             * Synchronous multiple topics subscription.
             * Subscriptions are represented as vector of tuples of topic name, quality-of-service and
             * read callback.  The topic names can be with or without wildcards. It allows to do single call
             * to subscribe to many topics. No special handling of overlapped subscriptions. No special
             * rules about order of topics in the list.
             * @param params  multiple tuples of 'topic', 'qos', 'slotFunction' to subscribe
             * @return boost::system::error_code
             */
            virtual boost::system::error_code subscribe(const TopicSubOptions& params) = 0;

            /**
             * Asynchronous multiple topics subscription.
             * This is an asynchronous version of synchronous multiple topics call. The call is non-blocking
             * and onComplete handler will be called after receiving broker acknowledgment.
             * @param params   multiple tuples of 'topic', 'qos', 'slotName', 'slotFunction' to subscribe
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            virtual void subscribeAsync(const TopicSubOptions& params, const AsyncHandler& onComplete) = 0;

            /**
             * Request broker to un-subscribe the topic.
             * If the topic not known then nothing is changed.
             * If un-subscribing is successful then the registered read callback associated
             * with the topic is removed, otherwise nothing is changed  .
             * @param topic to un-subscribe
             * @return boost::system::error_code indicating if un-subscription is successful
             */
            virtual boost::system::error_code unsubscribe(const std::string& topic) = 0;

            /**
             * Request broker to un-subscribe topic.
             * @param topic to un-subscribe
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            virtual void unsubscribeAsync(
                  const std::string& topic,
                  const AsyncHandler& onComplete = [](const boost::system::error_code&) {}) = 0;

            /**
             * Request broker to un-subscribe multiple topics
             * @param multiple topic-handlerName tuples to un-subscribe
             * @return boost::system::error_code indicating if un-subscription is successful (all or none)
             */
            virtual boost::system::error_code unsubscribe(const std::vector<std::string>& topics) = 0;

            /**
             * Request broker to un-subscribe many topics
             * @param multiple topics to un-subscribe
             * @param onComplete handler with signature "void (boost::system::error_code)"
             */
            virtual void unsubscribeAsync(
                  const std::vector<std::string>& topics,
                  const AsyncHandler& onComplete = [](const boost::system::error_code&) {}) = 0;

            /**
             * Un-subscribe from all subscriptions made by this client upto now (blocking call)
             * @return boost::system::error_code indicating if un-subscribing was successful (all or none)
             */
            virtual boost::system::error_code unsubscribeAll() = 0;

            /**
             * Un-subscribe from all subscriptions made by this client upto now (non-blocking call)
             * @param onUnSubAck acknowledgment callback will be called at the end
             * @param callback after network completion (send operation was successful)
             */
            virtual void unsubscribeAllAsync(const AsyncHandler& onComplete = [](const boost::system::error_code&) {
            }) = 0;

            /**
             * Check if the topic is subscribed already.
             * @param topic
             * @return true or false
             */
            virtual bool isSubscribed(const std::string& topic) const = 0;

            /**
             * Check if this topic is "matched", i.e. effectively subscribed.
             * It means that either exact subscription was already done before or
             * wildcards were used to subscribe to multiple topics and given topic
             * is one of them.
             *
             * @param topic to be matched
             * @return true or false
             */
            virtual bool isMatched(const std::string& topic) = 0;

            boost::system::error_code publish(const std::string& topic, const karabo::util::Hash::Pointer& msg,
                                              PubOpts pubopts) {
                return publish(topic, msg, std::uint8_t(pubopts));
            }

            /**
             * Publish a message (Hash) asynchronously on the topic with given options
             * @param topic to publish
             * @param msg   to publish
             * @param pubopts Example: PubQos::ExactlyOnce | PubRetain::yes
             * @param onComplete
             */
            void publishAsync(const std::string& topic, const karabo::util::Hash::Pointer& msg, PubOpts pubopts,
                              const AsyncHandler& onComplete = AsyncHandler()) {
                publishAsync(topic, msg, std::uint8_t(pubopts), onComplete);
            }

            virtual ReadHashHandler getReadHashHandler(const std::string& topic) const = 0;

            void setInstanceId(const std::string& instanceId) {
                m_instanceId = instanceId;
            }

            void setDomain(const std::string& domain) {
                m_domain = domain;
            }

            virtual std::string getClientId() const = 0;

            virtual const std::string& getBrokerUrl() const = 0;

            /**
             * Retrieve all topics we are subscribed to so far. Wildcards
             * subscriptions are book-kept as they were defined.
             * @return list of topics including wildcards
             */
            virtual std::vector<std::string> getSubscriptions() const = 0;

            /**
             * Retrieves all handlers (in most cases one) that were registered
             * to be called when the message from given topic is arrived
             * @param topic of interest
             * @return vector of registered handlers
             */
            virtual std::vector<ReadHashHandler> getSubscribeHandler(const std::string& topic) const = 0;

            static std::string getUuidAsString();

           protected:
            virtual boost::system::error_code subscribe(const std::string& topic, std::uint8_t subopts,
                                                        const ReadHashHandler& slotFunc) = 0;

            virtual void subscribeAsync(
                  const std::string& topic, std::uint8_t subopts, const ReadHashHandler& slotFunc,
                  const AsyncHandler& onComplete = [](const boost::system::error_code&) {}) = 0;

            /**
             * Publish a message (Hash) on the topic (blocking call) with QoS (Quality of Service) equal
             * 0 => at most once
             * 1 => at least once
             * 2 => exactly once
             * @param topic   topic string
             * @param qos     quality of service
             * @param msg     shared pointer to Hash
             * @param retain  flag enabling message persistence
             * @return boost::system::error_code
             */
            boost::system::error_code publish(const std::string& topic, int qos, const karabo::util::Hash::Pointer& msg,
                                              bool retain = false) {
                PubOpts options;
                if (qos == 0) options |= PubQos::AtMostOnce;
                else if (qos == 1) options |= PubQos::AtLeastOnce;
                else if (qos == 2) options |= PubQos::ExactlyOnce;
                if (retain) options |= PubRetain::yes;
                return publish(topic, msg, options);
            }

            /**
             * Helper variant for publish
             * @param topic
             * @param msg
             * @param pubopts
             * @return
             */
            virtual boost::system::error_code publish(const std::string& topic, const karabo::util::Hash::Pointer& msg,
                                                      std::uint8_t pubopts) = 0;

            virtual void publishAsync(const std::string& topic, const karabo::util::Hash::Pointer& msg,
                                      std::uint8_t pubopts, const AsyncHandler& onComplete = AsyncHandler()) = 0;

            /**
             * Publish a message (Hash) on the topic (nonblocking call) with QoS (Quality of Service) equal
             * 0 => at most once
             * 1 => at least once
             * 2 => exactly once
             * @param topic   topic string
             * @param qos     quality of service
             * @param msg     shared pointer to Hash
             * @param onComplete handler with signature "void (boost::system::error_code)"
             * @param retain  flag enabling message persistence
             */
            void publishAsync(const std::string& topic, int qos, const karabo::util::Hash::Pointer& msg,
                              const AsyncHandler& onComplete, bool retain = false) {
                PubOpts options;
                if (qos == 0) options |= PubQos::AtMostOnce;
                else if (qos == 1) options |= PubQos::AtLeastOnce;
                else if (qos == 2) options |= PubQos::ExactlyOnce;
                if (retain) options |= PubRetain::yes;
                publishAsync(topic, msg, options, onComplete);
            }

            /**
             * Dispatch functor on MQTT event loop
             * @param token
             */
            template <typename CompletionToken>
            void dispatch(CompletionToken&& token) {
                m_ios->dispatch(token);
            }

            template <typename CompletionToken>
            void post(CompletionToken&& token) {
                m_ios->post(token);
            }

           private:
            void run();

           protected:
            boost::shared_ptr<boost::asio::io_context> m_ios;
            boost::shared_ptr<boost::thread> m_thread;
            std::vector<std::string> m_brokerUrls;
            std::string m_domain;
            std::string m_instanceId;
            bool m_skipFlag;
        };
    } // namespace net
} // namespace karabo


#endif /* KARABO_NET_MQTTCLIENT_HH */
