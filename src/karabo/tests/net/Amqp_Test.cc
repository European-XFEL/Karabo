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
 * File:   Amqp_Test.cc
 *
 * Created on March 4th, 2024
 */


#include "Amqp_Test.hh"

#include <chrono>
#include <future>
#include <mutex>
#include <thread>

#include "karabo/data/types/StringTools.hh"
#include "karabo/log/Logger.hh"
#include "karabo/net/AmqpClient.hh"
#include "karabo/net/AmqpConnection.hh"
#include "karabo/net/AmqpHashClient.hh"
#include "karabo/net/AmqpUtils.hh"
#include "karabo/net/Broker.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/tests/BrokerUtils.hh"
#include "karabo/tests/WaitUtils.hh"

using namespace karabo;
using namespace std::chrono;
using namespace std::literals::chrono_literals;
using karabo::tests::waitForCondition;

CPPUNIT_TEST_SUITE_REGISTRATION(Amqp_Test);

std::atomic<bool> Amqp_Test::m_loggingIsSetup(false);

Amqp_Test::Amqp_Test()
    // Use the environment variable KARABO_CI_BROKERS to define the brokers to use in tests:
    // e.g. export KARABO_CI_BROKERS=amqp://an-amqp-broker:5672
    // If that variable is not defined, KARABO_BROKER with an amqp broker also works
    : m_defaultBrokers(getBrokerFromEnv("amqp")), m_timeout(10), m_timeoutMs(10'000) {
    if (false &&                            // remove 'false &&' to switch on logging
        !m_loggingIsSetup.exchange(true)) { // configure logging only once in the process
        log::Logger::configure(data::Hash("level", "DEBUG"));
        log::Logger::useConsole();
    }
}

Amqp_Test::~Amqp_Test() {}


void Amqp_Test::testConnection() {
    const std::string urlBadHostPort("amqp://wha:whu@invalidhost:5555");
    if (m_defaultBrokers.empty()) {
        std::clog << " No AMQP broker in environment. Skipping those connection tests that require it..." << std::flush;
    } else {
        // test asyncConnect - proper url
        net::AmqpConnection::Pointer connection(std::make_shared<net::AmqpConnection>(m_defaultBrokers));
        CPPUNIT_ASSERT(!connection->isConnected());
        CPPUNIT_ASSERT_EQUAL(std::string("AMQP::Connection is not yet created!"), connection->connectionInfo());
        auto done = std::make_shared<std::promise<boost::system::error_code>>();
        auto fut = done->get_future();
        connection->asyncConnect([done](const boost::system::error_code ec) { done->set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut.wait_for(m_timeout));
        const boost::system::error_code ec = fut.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), static_cast<int>(boost::system::errc::success), ec.value());
        CPPUNIT_ASSERT_EQUAL(m_defaultBrokers.front(), connection->getCurrentUrl());
        CPPUNIT_ASSERT(connection->isConnected());
        CPPUNIT_ASSERT_EQUAL(
              std::string("AMQP::Connection is usable, ready, initialized, not closed and has 0 channels."),
              connection->connectionInfo());
        // We can safely destruct the connection again.
        CPPUNIT_ASSERT_EQUAL(1l, connection.use_count());
        CPPUNIT_ASSERT_NO_THROW(connection.reset());

        // test asyncConnect - but improper user/password url
        std::string urlBadUser = m_defaultBrokers.front();
        const size_t endUserPasswd = urlBadUser.find('@'); // 7 is size of "amqp://"
        boost::replace_first(urlBadUser, urlBadUser.substr(7, endUserPasswd - 7), "invalid:user_password");
        connection = std::make_shared<net::AmqpConnection>(std::vector<std::string>(1, urlBadUser));
        auto done2 = std::make_shared<std::promise<boost::system::error_code>>();
        auto fut2 = done2->get_future();
        connection->asyncConnect([done2](const boost::system::error_code ec) { done2->set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut2.wait_for(m_timeout));
        const boost::system::error_code ec2 = fut2.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec2.message(), static_cast<int>(boost::system::errc::connection_refused),
                                     ec2.value());
        CPPUNIT_ASSERT_EQUAL(urlBadUser, connection->getCurrentUrl());
        CPPUNIT_ASSERT(!connection->isConnected());
        waitForCondition([&connection]() { return connection.use_count() == 1l; }, m_timeoutMs);
        CPPUNIT_ASSERT_EQUAL(1l, connection.use_count());
        CPPUNIT_ASSERT_NO_THROW(connection.reset());

        // test asyncConnect with > 1 urls, but all bad: will receive error code matching the last url
        // (first: last is urlBadHostPort)
        std::vector<std::string> urls({urlBadUser, urlBadHostPort});
        connection = std::make_shared<net::AmqpConnection>(urls);
        auto done4 = std::make_shared<std::promise<boost::system::error_code>>();
        auto fut4 = done4->get_future();
        connection->asyncConnect([done4](const boost::system::error_code ec) { done4->set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut4.wait_for(m_timeout));
        const boost::system::error_code ec4 = fut4.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec4.message(), // not_connected since last url is urlBadHostPort
                                     static_cast<int>(boost::system::errc::not_connected), ec4.value());
        // Note: All urls failed, so connection rolls back to first one again
        CPPUNIT_ASSERT_EQUAL(urls.front(), connection->getCurrentUrl());
        CPPUNIT_ASSERT(!connection->isConnected());
        CPPUNIT_ASSERT_EQUAL(1l, connection.use_count());
        CPPUNIT_ASSERT_NO_THROW(connection.reset());

        // (now: last is bad credentials)
        urls = {urlBadHostPort, urlBadUser};
        connection = std::make_shared<net::AmqpConnection>(urls);
        auto done5 = std::make_shared<std::promise<boost::system::error_code>>();
        auto fut5 = done5->get_future();
        connection->asyncConnect([done5](const boost::system::error_code ec) { done5->set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut5.wait_for(m_timeout));
        const boost::system::error_code ec5 = fut5.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec5.message(), // connection_refused since last url is 'urlBadUser'
                                     static_cast<int>(boost::system::errc::connection_refused), ec5.value());
        // Note: All urls failed, so connection rolls back to first one again
        CPPUNIT_ASSERT_EQUAL(urls.front(), connection->getCurrentUrl());
        CPPUNIT_ASSERT(!connection->isConnected());
        waitForCondition([&connection]() { return connection.use_count() == 1l; }, m_timeoutMs);
        CPPUNIT_ASSERT_EQUAL(1l, connection.use_count());
        CPPUNIT_ASSERT_NO_THROW(connection.reset());

        // Now test - 4 addresses, last but one is valid and
        //          - use many concurrent asyncConnect
        urls.push_back(m_defaultBrokers.front());
        urls.push_back(urls.front());
        connection = std::make_shared<net::AmqpConnection>(urls);
        auto dones3 = std::make_shared<std::vector<std::promise<boost::system::error_code>>>();
        std::vector<std::future<boost::system::error_code>> futs3;
        const int numConcurrentConnect = 50;
        dones3->reserve(numConcurrentConnect); // avoid reallocation of promise - concurrent access possible in handler
        for (int i = 0; i < numConcurrentConnect; ++i) {
            dones3->push_back(std::promise<boost::system::error_code>());
            futs3.push_back(dones3->back().get_future());
            connection->asyncConnect([dones3, i](const boost::system::error_code ec) { dones3->at(i).set_value(ec); });
            // Little sleep to have asyncConnect requests happen in different stages of creation of connection
            std::this_thread::sleep_for(500us);
        }
        for (int i = 0; i < numConcurrentConnect; ++i) {
            CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futs3[i].wait_for(3 * m_timeout));
            const boost::system::error_code ec3 = futs3[i].get();
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ec3.message(), static_cast<int>(boost::system::errc::success), ec3.value());
        }
        CPPUNIT_ASSERT_GREATEREQUAL(2ul, urls.size());
        CPPUNIT_ASSERT_EQUAL(urls[urls.size() - 2], connection->getCurrentUrl());
        CPPUNIT_ASSERT(connection->isConnected());

        // Here add test for successful channel creation
        auto doneCreation = std::make_shared<std::promise<std::shared_ptr<AMQP::Channel>>>();
        auto futCreateChannel = doneCreation->get_future();
        connection->asyncCreateChannel(
              [doneCreation](const std::shared_ptr<AMQP::Channel>& channel, const std::string& errMsg) {
                  if (!errMsg.empty()) doneCreation->set_value(nullptr);
                  else doneCreation->set_value(channel);
              });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futCreateChannel.wait_for(m_timeout));
        auto channel = futCreateChannel.get();
        CPPUNIT_ASSERT(channel);          // A channel has been created
        CPPUNIT_ASSERT(channel->ready()); // ... and is ready

        waitForCondition([&channel]() { return channel.use_count() == 1l; }, m_timeoutMs);
        CPPUNIT_ASSERT_EQUAL(1l, channel.use_count());
        CPPUNIT_ASSERT_NO_THROW(channel.reset());

        CPPUNIT_ASSERT_EQUAL(1l, connection.use_count());
        CPPUNIT_ASSERT_NO_THROW(connection.reset());

        // Now channel creation with lazy connection (i.e. connect if not yet connected)
        const int numChannels = 100;                              // Directly stress test with many channels
        connection = std::make_shared<net::AmqpConnection>(urls); // Still 3 urls, last one valid

        auto channelPromises = std::make_shared<std::vector<std::promise<std::string>>>();
        // Avoid re-allocations and thus data races when used in below lambda for given 'i' and push_back more promises:
        channelPromises->reserve(numChannels);
        std::vector<std::future<std::string>> channelFutures;
        for (int i = 0; i < numChannels; ++i) {
            channelPromises->push_back(std::promise<std::string>());
            channelFutures.push_back(channelPromises->back().get_future());
            connection->asyncCreateChannel(
                  [i, channelPromises](const std::shared_ptr<AMQP::Channel>& channel, const std::string& errMsg) {
                      if (channel) channelPromises->at(i).set_value("Channel created");
                      else channelPromises->at(i).set_value(errMsg);
                  });
            // Little sleep to have asyncCreateChannel requests happen in different stages of creation of connection
            std::this_thread::sleep_for(500us);
        }
        for (int i = 0; i < numChannels; ++i) {
            CPPUNIT_ASSERT_EQUAL(std::future_status::ready, channelFutures[i].wait_for(m_timeout));
            CPPUNIT_ASSERT_EQUAL(std::string("Channel created"), channelFutures[i].get());
        }
        CPPUNIT_ASSERT_EQUAL(1l, connection.use_count());
        CPPUNIT_ASSERT_NO_THROW(connection.reset());

        // Test creation of many, many channels and check that we get failure report (and no crash)
        const size_t maxNumChannels = 2047; // number that current lib version allows
        connection = std::make_shared<net::AmqpConnection>(m_defaultBrokers);
        std::vector<std::shared_ptr<AMQP::Channel>> channels;
        for (size_t i = 0; i < maxNumChannels + 1; ++i) { // '+1': try one more than will work
            auto doneCreation =
                  std::make_shared<std::promise<std::pair<std::shared_ptr<AMQP::Channel>, std::string>>>();
            auto futCreateChannel = doneCreation->get_future();
            connection->asyncCreateChannel(
                  [doneCreation](const std::shared_ptr<AMQP::Channel>& channel, const std::string& errMsg) {
                      doneCreation->set_value(std::pair(channel, errMsg));
                  });
            CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futCreateChannel.wait_for(m_timeout));
            const auto channel_msg = futCreateChannel.get();
            if (i < maxNumChannels) {
                CPPUNIT_ASSERT_MESSAGE("Channel missing: " + data::toString(i), channel_msg.first);
                channels.push_back(channel_msg.first); // keep alive
            } else {
                CPPUNIT_ASSERT_EQUAL(std::string("Runtime exception creating channel: failed to open channel: "
                                                 "max number of channels has been reached"),
                                     channel_msg.second);
            }
        }
        CPPUNIT_ASSERT_EQUAL(maxNumChannels, channels.size());
        channels.clear();
        CPPUNIT_ASSERT_EQUAL(1l, connection.use_count());
        CPPUNIT_ASSERT_NO_THROW(connection.reset());

        // Test that pending handlers (connect and create channel) are called in destructor
        connection = std::make_shared<net::AmqpConnection>(urls); // Still 3 urls, last one valid
        auto connDone = std::make_shared<std::promise<boost::system::error_code>>();
        auto connFut = connDone->get_future();
        connection->asyncConnect([connDone](const boost::system::error_code ec) { connDone->set_value(ec); });
        auto chanDone = std::make_shared<std::promise<std::string>>();
        auto chanFut = chanDone->get_future();
        connection->asyncCreateChannel(
              [chanDone](const std::shared_ptr<AMQP::Channel>& channel, const std::string& errMsg) {
                  if (channel) chanDone->set_value("Non empty channelPtr!");
                  else chanDone->set_value(errMsg);
              });
        // Ensure that the dispatched async actions got executed by waiting until one more dispatched function is done
        // (otherwise asyncCreateChannel handler might not yet have stored its pending channel creation).
        auto movedOnProm = std::make_shared<std::promise<void>>();
        auto movedOnFut = movedOnProm->get_future();
        connection->dispatch([movedOnProm]() { movedOnProm->set_value(); });
        movedOnFut.wait();

        // Rarely, direct check of use_count gave 2. Give other threads the chance to finish
        waitForCondition([&connection]() { return connection.use_count() == 1l; }, m_timeoutMs);
        CPPUNIT_ASSERT_EQUAL(1l, connection.use_count());
        CPPUNIT_ASSERT_NO_THROW(connection.reset());
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, connFut.wait_for(m_timeout));
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, chanFut.wait_for(m_timeout));
        const boost::system::error_code connEc = connFut.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(connEc.message(), static_cast<int>(boost::system::errc::operation_canceled),
                                     connEc.value());
        CPPUNIT_ASSERT_EQUAL(std::string("Connection destructed"), chanFut.get());
    }
    { // test invalid tcp address - the tests for post and dispatch sneaked in as well
        const std::vector<std::string> invalidIps(1, urlBadHostPort);
        net::AmqpConnection::Pointer connection(std::make_shared<net::AmqpConnection>(invalidIps));

        // first test post(..) and dispatch(..)
        auto donePost = std::make_shared<std::promise<void>>();
        auto futPost = donePost->get_future();
        connection->post([donePost]() { donePost->set_value(); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futPost.wait_for(m_timeout));
        futPost.get();

        auto doneDispatch = std::make_shared<std::promise<void>>();
        auto futDispatch = doneDispatch->get_future();
        connection->dispatch([doneDispatch]() { doneDispatch->set_value(); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futDispatch.wait_for(m_timeout));
        futDispatch.get();
        // TODO? Add a test that checks that dispatching a method means directly calling it if already in the
        // io_context?

        // Now the real test for invalid tcp address
        auto done = std::make_shared<std::promise<boost::system::error_code>>();
        auto fut = done->get_future();
        connection->asyncConnect([done](const boost::system::error_code ec) { done->set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut.wait_for(m_timeout));
        const boost::system::error_code ec = fut.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), static_cast<int>(boost::system::errc::not_connected), ec.value());
        CPPUNIT_ASSERT_EQUAL(urlBadHostPort, connection->getCurrentUrl());
        CPPUNIT_ASSERT(!connection->isConnected());

        // Also test failing channel creation because connection cannot be established
        auto doneCreation = std::make_shared<std::promise<std::string>>();
        auto futCreateChannel = doneCreation->get_future();

        connection->asyncCreateChannel(
              [doneCreation](const std::shared_ptr<AMQP::Channel>& channel, const std::string& errMsg) {
                  if (channel) doneCreation->set_value("Non empty channelPtr!");
                  else doneCreation->set_value(errMsg);
              });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futCreateChannel.wait_for(m_timeout));
        const std::string msg(futCreateChannel.get());
        CPPUNIT_ASSERT_MESSAGE(msg, msg.find("Connection could not be established") != std::string::npos);

        // We can safely destruct the connection again
        CPPUNIT_ASSERT(!connection->isConnected());
        waitForCondition([&connection]() { return connection.use_count() == 1l; }, m_timeoutMs);
        CPPUNIT_ASSERT_EQUAL(1l, connection.use_count());
        CPPUNIT_ASSERT_NO_THROW(connection.reset());
    }
    { // test wrongly formatted address
        net::AmqpConnection::Pointer connection(
              std::make_shared<net::AmqpConnection>(std::vector<std::string>(1, "not://proper:protocol")));
        auto done = std::make_shared<std::promise<boost::system::error_code>>();
        auto fut = done->get_future();
        connection->asyncConnect([done](const boost::system::error_code ec) { done->set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut.wait_for(m_timeout));
        const boost::system::error_code ec = fut.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), static_cast<int>(boost::system::errc::wrong_protocol_type),
                                     ec.value());
        CPPUNIT_ASSERT(!connection->isConnected());
        CPPUNIT_ASSERT_EQUAL(std::string("not://proper:protocol"), connection->getCurrentUrl());
        CPPUNIT_ASSERT_NO_THROW(connection.reset());
    }
}

void Amqp_Test::testClient() {
    if (m_defaultBrokers.empty()) {
        std::clog << " No AMQP broker in environment. Skipping client tests..." << std::endl;
        return;
    }

    // Prepare valid connection
    net::AmqpConnection::Pointer connection(std::make_shared<net::AmqpConnection>(m_defaultBrokers));
    auto done = std::make_shared<std::promise<boost::system::error_code>>();
    auto fut = done->get_future();
    connection->asyncConnect([done](const boost::system::error_code ec) { done->set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut.wait_for(m_timeout));
    const boost::system::error_code ec = fut.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), static_cast<int>(boost::system::errc::success), ec.value());

    // Create client "bob" with a read handler that simply appends data it reads to a container ('readByBob')
    auto readByBob =
          std::make_shared<std::vector<std::tuple<std::shared_ptr<std::vector<char>>, std::string, std::string>>>();
    auto readByBobCounter = std::make_shared<std::atomic<int>>(0);
    auto bobRead4 = std::make_shared<std::promise<void>>();
    auto futBobRead4 = bobRead4->get_future();
    auto readHandlerBob = [readByBob, readByBobCounter, bobRead4](const std::shared_ptr<std::vector<char>>& data,
                                                                  const std::string& exchange,
                                                                  const std::string& routingKey) {
        readByBob->push_back({data, exchange, routingKey});
        if (++(*readByBobCounter) == 4) {
            bobRead4->set_value();
        }
    };
    // To avoid interference of different test runs, any exchange and queue (i.e. nominal client instanceId) are
    // prefixed with broker domain (i.e. Karabo topic)
    const std::string prefix = net::Broker::brokerDomainFromEnv() += ".";
    net::AmqpClient::Pointer bob(
          std::make_shared<net::AmqpClient>(connection, prefix + "bob", AMQP::Table(), readHandlerBob));

    // In parallel subscribe twice while channel is created under the hood
    auto subDone1 = std::make_shared<std::promise<boost::system::error_code>>();
    auto fut1 = subDone1->get_future();
    bob->asyncSubscribe(prefix + "exchange1", "bob1",

                        [subDone1](const boost::system::error_code ec) { subDone1->set_value(ec); });

    auto subDone2 = std::make_shared<std::promise<boost::system::error_code>>();
    auto fut2 = subDone2->get_future();
    bob->asyncSubscribe(prefix + "exchange2", "bob2",
                        [subDone2](const boost::system::error_code ec) { subDone2->set_value(ec); });

    // Now wait for both subscriptions to be done
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut1.wait_for(m_timeout));
    const boost::system::error_code ec1 = fut1.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec1.message(), static_cast<int>(boost::system::errc::success), ec1.value());

    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut2.wait_for(m_timeout));
    const boost::system::error_code ec2 = fut2.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec2.message(), static_cast<int>(boost::system::errc::success), ec2.value());

    // Subscribe twice more after channel is already created
    auto subDone3 = std::make_shared<std::promise<boost::system::error_code>>();
    auto fut3 = subDone3->get_future();
    bob->asyncSubscribe(prefix + "exchange1", "bob3",
                        [subDone3](const boost::system::error_code ec) { subDone3->set_value(ec); });
    auto subDone4 = std::make_shared<std::promise<boost::system::error_code>>();
    auto fut4 = subDone4->get_future();
    bob->asyncSubscribe(prefix + "exchange2", "bob4",
                        [subDone4](const boost::system::error_code ec) { subDone4->set_value(ec); });

    // Again wait for both subscriptions to be done
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut3.wait_for(m_timeout));
    const boost::system::error_code ec3 = fut3.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec3.message(), static_cast<int>(boost::system::errc::success), ec3.value());

    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut4.wait_for(m_timeout));
    const boost::system::error_code ec4 = fut4.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec4.message(), static_cast<int>(boost::system::errc::success), ec4.value());

    // Now create 2nd client 'alice' and let it talk to 'bob' - no need to subscribe beforehand
    // Note that 'alice' will only receive one message at the very end.
    auto numReadAlice = std::make_shared<std::atomic<int>>(0);
    auto readHandlerAlice = [numReadAlice](const std::shared_ptr<std::vector<char>>& data, const std::string& exchange,
                                           const std::string& routingKey) { ++(*numReadAlice); };
    net::AmqpClient::Pointer alice(
          std::make_shared<net::AmqpClient>(connection, prefix + "alice", AMQP::Table(), readHandlerAlice));

    std::vector<std::future<boost::system::error_code>> publishFutures;
    for (size_t i = 0; i < 5; ++i) {
        auto publishDone = std::make_shared<std::promise<boost::system::error_code>>();
        publishFutures.push_back(publishDone->get_future());
        std::string routingKey("bob ");
        routingKey.back() = '1' + i; // "bob1", "bob2", ..., "bob5" - note that bob did not subscribe to "bob5"!
        std::string exchange(prefix + "exchange ");
        exchange.back() = (i % 2 == 0 ? '1' : '2'); // Use alternating exchanges
        alice->asyncPublish(exchange, routingKey, std::make_shared<std::vector<char>>(10, 'a' + i),
                            [publishDone{std::move(publishDone)}](const boost::system::error_code ec) {
                                publishDone->set_value(ec);
                            });
    }
    // Wait for confirmations of all published messages
    for (std::future<boost::system::error_code>& fut : publishFutures) {
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut.wait_for(m_timeout));
        const boost::system::error_code ec = fut.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), static_cast<int>(boost::system::errc::success), ec.value());
    }

    // Bob should have received the first four messages (and in order),
    // but not the fifth since bob did not subscribe to routingKey "bob5"
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futBobRead4.wait_for(m_timeout));
    CPPUNIT_ASSERT_EQUAL(4, readByBobCounter->load());
    CPPUNIT_ASSERT_EQUAL(4ul, readByBob->size());
    for (size_t i = 0; i < readByBob->size(); ++i) {
        std::string exchange(prefix + "exchange ");
        exchange.back() = (i % 2 == 0 ? '1' : '2'); // Same alternating exchanges as for sending
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Message " + karabo::data::toString(i), exchange, std::get<1>(readByBob->at(i)));
        std::string routingKey("bob ");
        routingKey.back() = '1' + i; // "bob1", "bob2", "bob3", "bob4"
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Message " + karabo::data::toString(i), routingKey, std::get<2>(readByBob->at(i)));
        // Check data content
        const std::shared_ptr<std::vector<char>>& data = std::get<0>(readByBob->at(i));
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Message " + karabo::data::toString(i), 10ul, data->size());
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Message " + karabo::data::toString(i), static_cast<char>('a' + i), (*data)[0]);
    }

    // Give some time for the fifth message - though it should not come
    std::this_thread::sleep_for(100ms);
    CPPUNIT_ASSERT_EQUAL(4ul, readByBob->size());

    //***************************************************************
    // Now test alice subscribing and bob publishing (other order between subscription and publish than before)
    auto subDoneAlice = std::make_shared<std::promise<boost::system::error_code>>();
    auto futAlice = subDoneAlice->get_future();
    alice->asyncSubscribe(prefix + "other_exchange", "alice",
                          [subDoneAlice](const boost::system::error_code ec) { subDoneAlice->set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futAlice.wait_for(m_timeout));
    const boost::system::error_code ecAlice = futAlice.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecAlice.message(), static_cast<int>(boost::system::errc::success), ecAlice.value());

    // Simply check that a message arrives after max. 2 seconds...
    CPPUNIT_ASSERT_EQUAL(0, numReadAlice->load());
    bob->asyncPublish(prefix + "other_exchange", "alice", std::make_shared<std::vector<char>>(5, 'b'),
                      [](const boost::system::error_code ec) {});
    for (unsigned int i = 0; i < m_timeoutMs; ++i) {
        if (*numReadAlice >= 1) break;
        std::this_thread::sleep_for(1ms);
    }
    CPPUNIT_ASSERT_EQUAL(1, numReadAlice->load());

    // Now check that read handler can be changed (intended only for postponed setting, though)
    auto numNewReadAlice = std::make_shared<std::atomic<int>>(0);
    alice->setReadHandler([numNewReadAlice](const std::shared_ptr<std::vector<char>>& data, const std::string& exchange,
                                            const std::string& routingKey) { ++(*numNewReadAlice); });
    bob->asyncPublish(prefix + "other_exchange", "alice", std::make_shared<std::vector<char>>(6, 'c'),
                      [](const boost::system::error_code ec) {});
    for (unsigned int i = 0; i < m_timeoutMs; ++i) {
        if (*numNewReadAlice >= 1) break;
        std::this_thread::sleep_for(1ms);
    }
    CPPUNIT_ASSERT_EQUAL(1, numNewReadAlice->load());
    // Even with some extra time for message travel, old handler does not receive
    std::this_thread::sleep_for(100ms);
    CPPUNIT_ASSERT_EQUAL(1, numReadAlice->load()); // as before

    // Cannot set an invalid read handler
    CPPUNIT_ASSERT_THROW(alice->setReadHandler(net::AmqpClient::ReadHandler()), karabo::data::ParameterException);

    //***************************************************************
    // Now test unsubscribing
    auto unsubDoneAlice = std::make_shared<std::promise<boost::system::error_code>>();
    auto futUnsubAlice = unsubDoneAlice->get_future();
    alice->asyncUnsubscribe(prefix + "other_exchange", "alice",
                            [unsubDoneAlice](const boost::system::error_code ec) { unsubDoneAlice->set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futUnsubAlice.wait_for(m_timeout));
    const boost::system::error_code ecAliceUnsub = futUnsubAlice.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecAliceUnsub.message(), static_cast<int>(boost::system::errc::success),
                                 ecAliceUnsub.value());

    //***************************************************************
    // Test that, after alice has unsubscribed above, it does not receive further messages
    auto writeDoneBob = std::make_shared<std::promise<boost::system::error_code>>();
    auto futWriteBob = writeDoneBob->get_future();
    bob->asyncPublish(prefix + "other_exchange", "alice", std::make_shared<std::vector<char>>(4, 'c'),
                      [writeDoneBob](const boost::system::error_code ec) { writeDoneBob->set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futWriteBob.wait_for(m_timeout));
    const boost::system::error_code ecBobWrite = futWriteBob.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecBobWrite.message(), static_cast<int>(boost::system::errc::success),
                                 ecBobWrite.value());

    std::this_thread::sleep_for(100ms);               // Grant some message travel time...
    CPPUNIT_ASSERT_EQUAL(1, numNewReadAlice->load()); // ...but nothing arrives due to unsubscription!

    //***************************************************************
    // Test unsubscription of something not subscribed - gives success (though that is debatable)
    auto unsubDoneAlice2 = std::make_shared<std::promise<boost::system::error_code>>();
    auto futUnsubAlice2 = unsubDoneAlice2->get_future();
    alice->asyncUnsubscribe(prefix + "other_exchange", "not_subscribed_routing_key",
                            [unsubDoneAlice2](const boost::system::error_code ec) { unsubDoneAlice2->set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futUnsubAlice2.wait_for(m_timeout));
    const boost::system::error_code ecAliceUnsub2 = futUnsubAlice2.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecAliceUnsub2.message(), static_cast<int>(boost::system::errc::success),
                                 ecAliceUnsub2.value());

    //***************************************************************
    // Little concurrency test: Subscribing and immediately unsubscribing works
    auto subDoneAlice2 = std::make_shared<std::promise<boost::system::error_code>>();
    auto futSubAlice2 = subDoneAlice2->get_future();
    auto unsubDoneAlice3 = std::make_shared<std::promise<boost::system::error_code>>();
    auto futUnsubAlice3 = unsubDoneAlice3->get_future();
    alice->asyncSubscribe(prefix + "other_exchange", "alice",
                          [subDoneAlice2](const boost::system::error_code ec) { subDoneAlice2->set_value(ec); });
    alice->asyncUnsubscribe(prefix + "other_exchange", "alice",
                            [unsubDoneAlice3](const boost::system::error_code ec) { unsubDoneAlice3->set_value(ec); });

    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futSubAlice2.wait_for(m_timeout));
    const boost::system::error_code ecSubAlice2 = futSubAlice2.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecSubAlice2.message(), static_cast<int>(boost::system::errc::success),
                                 ecSubAlice2.value());
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futUnsubAlice3.wait_for(m_timeout));
    const boost::system::error_code ecAliceUnsub3 = futUnsubAlice3.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecAliceUnsub3.message(), static_cast<int>(boost::system::errc::success),
                                 ecAliceUnsub3.value());
    // And still, alice does not receive Bob's message
    auto writeDoneBob2 = std::make_shared<std::promise<boost::system::error_code>>();
    auto futWriteBob2 = writeDoneBob2->get_future();
    bob->asyncPublish(prefix + "other_exchange", "alice", std::make_shared<std::vector<char>>(4, 'd'),
                      [writeDoneBob2](const boost::system::error_code ec) { writeDoneBob2->set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futWriteBob2.wait_for(m_timeout));
    const boost::system::error_code ecBobWrite2 = futWriteBob2.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecBobWrite2.message(), static_cast<int>(boost::system::errc::success),
                                 ecBobWrite2.value());

    std::this_thread::sleep_for(100ms);               // Grant some message travel time...
    CPPUNIT_ASSERT_EQUAL(1, numNewReadAlice->load()); // ...but nothing arrives due to unsubscription!

    //***************************************************************
    // Test sending a message to an exchange that does not yet exist
    auto writeNonExistDone = std::make_shared<std::promise<boost::system::error_code>>();
    auto futWriteNonExist = writeNonExistDone->get_future();
    // TODO: Consider using a uuid as exchange suffix: Then repeated test runs would always see a new exchange. But
    //       that would pollute brokers (since exchanges are permanent) with exchanges even if the prefix stays
    bob->asyncPublish(prefix + "not_an_exchange", std::string(), std::make_shared<std::vector<char>>(4, 'y'),
                      [writeNonExistDone](const boost::system::error_code ec) { writeNonExistDone->set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futWriteNonExist.wait_for(m_timeout));
    const boost::system::error_code ecWriteNonExist = futWriteNonExist.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecWriteNonExist.message(), static_cast<int>(boost::system::errc::success),
                                 ecWriteNonExist.value());

    //***************************************************************
    // Test that bob can still use its channel to publish after publishing to a so far unknown exchange
    // So first subscribe alice again
    auto subDoneAlice3 = std::make_shared<std::promise<boost::system::error_code>>();
    auto futSubAlice3 = subDoneAlice3->get_future();
    alice->asyncSubscribe(prefix + "other_exchange", "alice",
                          [subDoneAlice3](const boost::system::error_code ec) { subDoneAlice3->set_value(ec); });
    const boost::system::error_code ecSubAlice3 = futSubAlice3.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecSubAlice3.message(), static_cast<int>(boost::system::errc::success),
                                 ecSubAlice3.value());
    CPPUNIT_ASSERT_EQUAL(1, numNewReadAlice->load()); // Just remember, so far one message to new handler

    // Now bob publishes
    auto writeDoneBob3 = std::make_shared<std::promise<boost::system::error_code>>();
    auto futWriteBob3 = writeDoneBob3->get_future();
    bob->asyncPublish(prefix + "other_exchange", "alice", std::make_shared<std::vector<char>>(4, 'z'),
                      [writeDoneBob3](const boost::system::error_code ec) { writeDoneBob3->set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futWriteBob3.wait_for(m_timeout));
    const boost::system::error_code ecWriteBob3 = futWriteBob3.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecWriteBob3.message(), static_cast<int>(boost::system::errc::success),
                                 ecWriteBob3.value());

    for (unsigned int i = 0; i < m_timeoutMs; ++i) {
        if (*numNewReadAlice >= 2) break;
        std::this_thread::sleep_for(1ms);
    }
    CPPUNIT_ASSERT_EQUAL(2, numNewReadAlice->load());
}

void Amqp_Test::testClientConcurrentSubscripts() {
    // Test concurrent [un]subscriptions to the same exchange/routingKey
    if (m_defaultBrokers.empty()) {
        std::clog << " No AMQP broker in environment. Skipping client tests for concurrent subscriptions..."
                  << std::endl;
        return;
    }

    // Prepare connection - will get connected automatically once clients need that
    net::AmqpConnection::Pointer connection(std::make_shared<net::AmqpConnection>(m_defaultBrokers));

    const std::string prefix = net::Broker::brokerDomainFromEnv() += ".";
    auto readCount = std::make_shared<std::atomic<int>>(0);
    net::AmqpClient::ReadHandler readerBob([readCount](const std::shared_ptr<std::vector<char>>& data,
                                                       const std::string& exch,
                                                       const std::string& key) { ++(*readCount); });
    net::AmqpClient::Pointer bob(
          std::make_shared<net::AmqpClient>(connection, prefix + "bob", AMQP::Table(), readerBob));
    const std::string exchange(prefix + "exchange");

    // Many subsequent subscriptions of the same, intermangled with an unsubscriptions and some sleeps in between:
    // This triggers the different code paths in AmqpClient::asyncSubscribe for already existing subscriptions.
    const size_t nSubscriptions = 200;
    auto subPromises = std::make_shared<std::vector<std::promise<boost::system::error_code>>>();
    std::vector<std::future<boost::system::error_code>> subFutures;
    for (size_t i = 0; i < nSubscriptions; ++i) {
        subPromises->push_back(std::promise<boost::system::error_code>());
        subFutures.push_back(subPromises->back().get_future());
        net::AsyncHandler callback(
              [i, subPromises](const boost::system::error_code ec) { subPromises->at(i).set_value(ec); });
        // All but one subscribe, unsubscription is already after 20% to ensure that at the end we are subscribed.
        if (i != nSubscriptions / 5) {
            bob->asyncSubscribe(exchange, "", callback);
        } else {
            bob->asyncUnsubscribe(exchange, "", callback);
        }
        std::this_thread::sleep_for(500us);
    }

    for (size_t i = 0; i < nSubscriptions; ++i) {
        auto& fut = subFutures[i];
        const std::string iStr("[Un]Subscription " + data::toString(i) += " ");
        CPPUNIT_ASSERT_EQUAL_MESSAGE(iStr, std::future_status::ready, fut.wait_for(m_timeout));
        boost::system::error_code ec = fut.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(iStr + ec.message(), 0, ec.value());
    }

    // Now send a message (sender is bob himself...) and check that it arrives only once,
    // although there are about  nSubscriptions/2 subscriptions after last unsubscription
    bob->asyncPublish(exchange, "", std::make_shared<std::vector<char>>(8, 'i'),
                      [](const boost::system::error_code ec) {});
    // Wait until the one expected message arrived and then 100 ms more for any further
    for (unsigned int i = 0; i < m_timeoutMs; ++i) {
        if (readCount->load() >= 1) break;
        std::this_thread::sleep_for(1ms);
    }
    std::this_thread::sleep_for(100ms);
    // If this fails, something may have gone wrong and the unsubscribe got executed last (How that?):
    CPPUNIT_ASSERT_EQUAL(1, readCount->load());
}

void Amqp_Test::testClientSameId() {
    if (m_defaultBrokers.empty()) {
        std::clog << " No AMQP broker in environment. Skipping client tests for same id..." << std::endl;
        return;
    }
    // Prepare valid connection
    net::AmqpConnection::Pointer connection(std::make_shared<net::AmqpConnection>(m_defaultBrokers));
    auto done = std::make_shared<std::promise<boost::system::error_code>>();
    auto fut = done->get_future();
    connection->asyncConnect([done](const boost::system::error_code ec) { done->set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut.wait_for(m_timeout));
    const boost::system::error_code ec = fut.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), static_cast<int>(boost::system::errc::success), ec.value());

    // To avoid interference of different test runs, any exchange and queue (i.e. nominal client instanceId) are
    // prefixed with broker domain (i.e. Karabo topic)
    const std::string prefix = net::Broker::brokerDomainFromEnv() += ".";


    // Create first client with id "bob"
    auto bobReadDone = std::make_shared<std::promise<std::shared_ptr<std::vector<char>>>>();
    auto bobReadFut = bobReadDone->get_future();
    auto readHandlerBob = [bobReadDone](const std::shared_ptr<std::vector<char>>& data, const std::string& exchange,
                                        const std::string& routingKey) { bobReadDone->set_value(data); };
    net::AmqpClient::Pointer bob(
          std::make_shared<net::AmqpClient>(connection, prefix + "bob", AMQP::Table(), readHandlerBob));

    auto bobSubDone = std::make_shared<std::promise<boost::system::error_code>>();
    auto bobSubFut = bobSubDone->get_future();
    bob->asyncSubscribe(prefix + "exchange", "bob",
                        [bobSubDone](const boost::system::error_code ec) { bobSubDone->set_value(ec); });
    const boost::system::error_code bobSubEc = bobSubFut.get(); // now directly get(), tired of m_timeout
    CPPUNIT_ASSERT_EQUAL_MESSAGE(bobSubEc.message(), static_cast<int>(boost::system::errc::success), bobSubEc.value());

    //***************************************************************
    // Create another client again with id "bob"
    std::vector<std::tuple<std::shared_ptr<std::vector<char>>, std::string, std::string>> readByBob2;
    auto bob2ReadDone = std::make_shared<std::promise<std::shared_ptr<std::vector<char>>>>();
    auto bob2ReadFut = bob2ReadDone->get_future();
    auto readHandlerBob2 = [bob2ReadDone](const std::shared_ptr<std::vector<char>>& data, const std::string& exchange,
                                          const std::string& routingKey) { bob2ReadDone->set_value(data); };
    net::AmqpClient::Pointer bob2( // Same id (prefix + "bob") as above!
          std::make_shared<net::AmqpClient>(connection, prefix + "bob", AMQP::Table(), readHandlerBob2));

    auto bob2SubDone = std::make_shared<std::promise<boost::system::error_code>>();
    auto bob2SubFut = bob2SubDone->get_future();
    bob2->asyncSubscribe(prefix + "exchange", "bob", // subscribe to same as other bob
                         [bob2SubDone](const boost::system::error_code ec) { bob2SubDone->set_value(ec); });
    const boost::system::error_code bob2SubEc = bob2SubFut.get(); // now directly get(), tired of m_timeout
    CPPUNIT_ASSERT_EQUAL_MESSAGE(bob2SubEc.message(), static_cast<int>(boost::system::errc::success),
                                 bob2SubEc.value());

    // Publish a message that both should receive (sender also receives, that does not matter)
    bob->asyncPublish(prefix + "exchange", "bob", std::make_shared<std::vector<char>>(4, 'x'),
                      [](const boost::system::error_code ec) {}); // do not care about success of sending
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, bobReadFut.wait_for(m_timeout));
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, bob2ReadFut.wait_for(m_timeout));
    const std::shared_ptr<std::vector<char>> bobReadData = bobReadFut.get();
    const std::shared_ptr<std::vector<char>> bob2ReadData = bob2ReadFut.get();
    CPPUNIT_ASSERT_EQUAL(4ul, bobReadData->size());
    CPPUNIT_ASSERT_EQUAL(4ul, bob2ReadData->size());
    CPPUNIT_ASSERT_EQUAL('x', bobReadData->front());
    CPPUNIT_ASSERT_EQUAL('x', bob2ReadData->front());

    //***************************************************************
    // Now stress testing with many clients with same id created in 'parallel' (well, one after another in the single
    // threaded io context of the connection, but the channel creation and subscription steps will be interleaved).
    // That will trigger different code paths that avoid queue id collisions in different steps.
    const int numClients = 20;

    // First create the clients
    std::vector<net::AmqpClient::Pointer> clients;
    auto receivedFlags = std::make_shared<std::vector<std::atomic<int>>>(numClients);
    for (int i = 0; i < numClients; ++i) {
        auto readHandler = [i, receivedFlags](const std::shared_ptr<std::vector<char>>& data,
                                              const std::string& exchange,
                                              const std::string& routingKey) { ++(receivedFlags->at(i)); };
        clients.push_back(std::make_shared<net::AmqpClient>(connection, prefix + "alice", AMQP::Table(), readHandler));
    }

    // Now let them all subscribe (and thus create channel, queue and consumer) in parallel
    auto subPromises = std::make_shared<std::vector<std::promise<boost::system::error_code>>>();
    std::vector<std::future<boost::system::error_code>> subFutures;
    for (int i = 0; i < numClients; ++i) {
        subPromises->push_back(std::promise<boost::system::error_code>());
        subFutures.push_back(subPromises->back().get_future());
        clients[i]->asyncSubscribe(
              prefix + "exchange", "alice", // subscribe to same for all
              [i, subPromises](const boost::system::error_code ec) { subPromises->at(i).set_value(ec); });
    }
    // Block until all confirm subscription
    for (int i = 0; i < numClients; ++i) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Future of client " + karabo::data::toString(i), std::future_status::ready,
                                     subFutures[i].wait_for(m_timeout));
        const boost::system::error_code ec = subFutures[i].get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE((karabo::data::toString(i) += ": ") + ec.message(),
                                     static_cast<int>(boost::system::errc::success), ec.value());
    }

    // Now send a message and verify its arrival at all clients
    clients[0]->asyncPublish(prefix + "exchange", "alice", std::make_shared<std::vector<char>>(4, 'a'),
                             [](const boost::system::error_code ec) {});
    bool allReceivedOne = false;
    for (int i = 0; i < 2000; ++i) {
        allReceivedOne = true;
        for (int iClient = 0; iClient < numClients; ++iClient) {
            const int nReceived = (*receivedFlags)[iClient];
            CPPUNIT_ASSERT_LESSEQUAL(1, nReceived); // receiving twice is a bug
            if (nReceived != 1) {
                allReceivedOne = false;
                break; // no need to check other clients
            }
        }
        if (allReceivedOne) break;
        else std::this_thread::sleep_for(1ms);
    }
    CPPUNIT_ASSERT(allReceivedOne);

    // Also test weird ids:
    // Amqp (client lib or rabbit broker itself?) will sanitize \r and \n away when declaring a queue with a name.
    // Routing keyes seem fine.
    std::vector<std::string> weirdIds(
          {"LF_END\n", "LF\nMIDDLE", "CR_END\r", "CR\rMIDDLE", "TAB_END\t", "TAB\tMIDDLE"}); // tabs look untouched
    for (const std::string& weirdId : weirdIds) {
        auto weirdReadDone = std::make_shared<std::promise<std::shared_ptr<std::vector<char>>>>();
        auto weirdReadFut = weirdReadDone->get_future();
        auto readHandlerWeird = [weirdReadDone](const std::shared_ptr<std::vector<char>>& data,
                                                const std::string& exchange,
                                                const std::string& routingKey) { weirdReadDone->set_value(data); };
        net::AmqpClient::Pointer weird(
              std::make_shared<net::AmqpClient>(connection, prefix + weirdId, AMQP::Table(), readHandlerWeird));

        auto weirdSubDone = std::make_shared<std::promise<boost::system::error_code>>();
        auto weirdSubFut = weirdSubDone->get_future();
        weird->asyncSubscribe(prefix + "exchange", weirdId, //"some",
                              [weirdSubDone](const boost::system::error_code ec) { weirdSubDone->set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, weirdSubFut.wait_for(m_timeout));
        const boost::system::error_code weirdSubEc = weirdSubFut.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(weirdSubEc.message(), static_cast<int>(boost::system::errc::success),
                                     weirdSubEc.value());

        // Write a message and verify it is received
        clients[0]->asyncPublish(prefix + "exchange", weirdId, std::make_shared<std::vector<char>>(4, 'a'),
                                 [](const boost::system::error_code ec) {});
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, weirdReadFut.wait_for(m_timeout));
        const std::shared_ptr<std::vector<char>> weirdReadData = weirdReadFut.get();
        CPPUNIT_ASSERT_EQUAL(4ul, weirdReadData->size());
        CPPUNIT_ASSERT_EQUAL('a', weirdReadData->at(0));
    }
}

void Amqp_Test::testClientUnsubscribeAll() {
    // Test asyncUnsubscribeAll
    if (m_defaultBrokers.empty()) {
        std::clog << " No AMQP broker in environment. Skipping client tests for unsubscribe all..." << std::endl;
        return;
    }

    // Prepare connection - will get connected automatically once clients need that
    net::AmqpConnection::Pointer connection(std::make_shared<net::AmqpConnection>(m_defaultBrokers));

    const std::string prefix = net::Broker::brokerDomainFromEnv() += ".";
    auto readCount = std::make_shared<std::atomic<size_t>>(0);
    net::AmqpClient::ReadHandler readerBob([readCount](const std::shared_ptr<std::vector<char>>& data,
                                                       const std::string& exch,
                                                       const std::string& key) { ++(*readCount); });
    net::AmqpClient::Pointer bob(
          std::make_shared<net::AmqpClient>(connection, prefix + "bob", AMQP::Table(), readerBob));
    const std::string exchange(prefix + "exchange");

    const size_t nSubscriptions = 20;
    auto subPromises = std::make_shared<std::vector<std::promise<boost::system::error_code>>>();
    std::vector<std::future<boost::system::error_code>> subFutures;
    for (size_t i = 0; i < nSubscriptions; ++i) {
        subPromises->push_back(std::promise<boost::system::error_code>());
        subFutures.push_back(subPromises->back().get_future());
        net::AsyncHandler callback(
              [i, subPromises](const boost::system::error_code ec) { (*subPromises)[i].set_value(ec); });
        bob->asyncSubscribe(exchange, "forBob_" + data::toString(i), callback);
    }

    for (size_t i = 0; i < nSubscriptions; ++i) {
        auto& fut = subFutures[i];
        const std::string iStr("Subscription " + data::toString(i));
        CPPUNIT_ASSERT_EQUAL_MESSAGE(iStr, std::future_status::ready, fut.wait_for(m_timeout));
        boost::system::error_code ec = fut.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(iStr + ec.message(), 0, ec.value());
    }

    // Now send a message (sender is bob himself...) for each subscription
    for (size_t i = 0; i < nSubscriptions; ++i) {
        bob->asyncPublish(exchange, "forBob_" + data::toString(i), std::make_shared<std::vector<char>>(i, 'i'),
                          [](const boost::system::error_code ec) {});
    }
    // Wait until they all arrived
    for (unsigned int i = 0; i < m_timeoutMs; ++i) {
        if (readCount->load() >= nSubscriptions) break;
        std::this_thread::sleep_for(1ms);
    }
    CPPUNIT_ASSERT_EQUAL(nSubscriptions, readCount->load());

    // Now unsubscribe all, send another message to each subscribed exchange/routing key and check that nothing arrives
    // anymore
    auto unsubAllDone = std::make_shared<std::promise<boost::system::error_code>>();
    auto futUnsubAll = unsubAllDone->get_future();
    bob->asyncUnsubscribeAll([unsubAllDone](const boost::system::error_code ec) { unsubAllDone->set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futUnsubAll.wait_for(m_timeout));
    const boost::system::error_code ec = futUnsubAll.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), 0, ec.value());

    for (size_t i = 0; i < nSubscriptions; ++i) {
        bob->asyncPublish(exchange, "forBob_" + data::toString(i), std::make_shared<std::vector<char>>(i, 'i'),
                          [](const boost::system::error_code ec) {});
    }

    // Even after sleeping, nothing more has arrived
    std::this_thread::sleep_for(100ms);
    CPPUNIT_ASSERT_EQUAL(nSubscriptions, readCount->load());
}

void Amqp_Test::testClientTooBigMessage() {
    // Test that client stays connected if a message beyond limits is sent
    if (m_defaultBrokers.empty()) {
        std::clog << " No AMQP broker in environment. Skipping client tests for too big message..." << std::endl;
        return;
    }

    // Prepare connection - will get connected automatically once clients need that
    net::AmqpConnection::Pointer connection(net::AmqpConnection::MakeShared(m_defaultBrokers));

    // Create alice (for sending) and bob (for reading ) clients
    const std::string prefix = net::Broker::brokerDomainFromEnv() += ".";
    std::shared_ptr<std::vector<char>> readMsg; // empty
    net::AmqpClient::ReadHandler readerBob([&readMsg](const std::shared_ptr<std::vector<char>>& data,
                                                      const std::string& exch,
                                                      const std::string& key) { readMsg = data; });
    net::AmqpClient::Pointer bob(net::AmqpClient::MakeShared(connection, prefix + "bob", AMQP::Table(), readerBob));
    net::AmqpClient::Pointer alice(net::AmqpClient::MakeShared(connection, prefix + "alice", AMQP::Table(),
                                                               net::AmqpClient::ReadHandler())); // alice won't read

    //  Subscribe bob
    const std::string exchange(prefix + "exchange");
    auto subPromise = std::make_shared<std::promise<boost::system::error_code>>();
    std::future<boost::system::error_code> subFuture = subPromise->get_future();
    net::AsyncHandler callback([subPromise](const boost::system::error_code ec) { subPromise->set_value(ec); });
    bob->asyncSubscribe(exchange, "forBob", callback);

    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, subFuture.wait_for(m_timeout));
    boost::system::error_code ec = subFuture.get();
    CPPUNIT_ASSERT_EQUAL(0, ec.value());

    // Now send a big, but supported message
    const size_t supportedSize = net::AmqpClient::m_maxMessageSize; //  134'217'728ul;
    auto pubPromise = std::make_shared<std::promise<boost::system::error_code>>();
    std::future<boost::system::error_code> pubFuture = pubPromise->get_future();
    alice->asyncPublish(exchange, "forBob", std::make_shared<std::vector<char>>(supportedSize, 'a'),
                        [pubPromise](const boost::system::error_code ec) { pubPromise->set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, pubFuture.wait_for(m_timeout));
    ec = pubFuture.get();
    CPPUNIT_ASSERT_EQUAL(0, ec.value());

    // Wait until message arrived and check it
    for (unsigned int i = 0; i < m_timeoutMs; ++i) {
        if (readMsg) break;
        std::this_thread::sleep_for(1ms);
    }
    CPPUNIT_ASSERT(readMsg);
    CPPUNIT_ASSERT_EQUAL(supportedSize, readMsg->size());
    CPPUNIT_ASSERT_EQUAL('a', readMsg->at(0));
    readMsg.reset(); // for next message

    // Now send the message that is too big (but where the client will notice directly).
    pubPromise = std::make_shared<std::promise<boost::system::error_code>>();
    pubFuture = pubPromise->get_future();
    const size_t nTooBig = supportedSize + 1ul; // Should be 134,217,729...
    alice->asyncPublish(exchange, "forBob", std::make_shared<std::vector<char>>(nTooBig, 'b'),
                        [pubPromise](const boost::system::error_code ec) { pubPromise->set_value(ec); });

    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, pubFuture.wait_for(m_timeout));
    ec = pubFuture.get();
    CPPUNIT_ASSERT_EQUAL(KARABO_ERROR_CODE_IO_ERROR.value(), ec.value());

    // Bad message will not arrive - but give it a bit of time
    std::this_thread::sleep_for(50ms);
    CPPUNIT_ASSERT(!readMsg);

    // Now send again a supported message size.
    pubPromise = std::make_shared<std::promise<boost::system::error_code>>();
    pubFuture = pubPromise->get_future();
    alice->asyncPublish(exchange, "forBob", std::make_shared<std::vector<char>>(11, 'c'),
                        [pubPromise](const boost::system::error_code ec) { pubPromise->set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, pubFuture.wait_for(m_timeout));
    ec = pubFuture.get();
    CPPUNIT_ASSERT_EQUAL(0, ec.value());

    // Wait until received
    for (unsigned int i = 0; i < m_timeoutMs; ++i) {
        if (readMsg) break;
        std::this_thread::sleep_for(1ms);
    }
    CPPUNIT_ASSERT(readMsg);
    CPPUNIT_ASSERT_EQUAL(11ul, readMsg->size());
    CPPUNIT_ASSERT_EQUAL('c', readMsg->at(0));
    readMsg.reset(); // for next use

    // Now manipulate AmqpClient to not catch a too big message - but let broker complain.
    // In this case, the channel will get unusable after a delay. So several more message can be sent
    // (and get lost!), but broker communication gets revived.
    net::AmqpClient::m_maxMessageSize += 1;
    pubPromise = std::make_shared<std::promise<boost::system::error_code>>();
    pubFuture = pubPromise->get_future();
    alice->asyncPublish(exchange, "forBob", std::make_shared<std::vector<char>>(nTooBig, 'd'),
                        [pubPromise](const boost::system::error_code ec) { pubPromise->set_value(ec); });

    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, pubFuture.wait_for(m_timeout));
    ec = pubFuture.get();
    CPPUNIT_ASSERT_EQUAL(0, ec.value()); // Unfortunately, success is claimed!

    // Bad message will not arrive - but give it a bit of time
    std::this_thread::sleep_for(50ms);
    CPPUNIT_ASSERT(!readMsg);

    // Unfortunately, it takes a while until the broker/AMQP lib will notice the too big message
    // and make the channel unusable. All messages published till then are lost.
    // We send until one arrives, i.e. we know that channel was recovered.
    unsigned int iSent = 0;
    for (; iSent < m_timeoutMs; ++iSent) {
        pubPromise = std::make_shared<std::promise<boost::system::error_code>>();
        pubFuture = pubPromise->get_future();
        alice->asyncPublish(exchange, "forBob", std::make_shared<std::vector<char>>(iSent + 1, 'e'),
                            [pubPromise](const boost::system::error_code ec) { pubPromise->set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, pubFuture.wait_for(m_timeout));
        ec = pubFuture.get();
        CPPUNIT_ASSERT_EQUAL(0, ec.value());
        if (readMsg) break;
        std::this_thread::sleep_for(1ms);
    }
    CPPUNIT_ASSERT(readMsg);
    CPPUNIT_ASSERT_EQUAL('e', readMsg->at(0));
    // Locally using our office broker the size of the message is about 775.
    // I also see that there are about 2.5 seconds between my try to send the too big message and the
    // responding log message from AmqpClient::channelErrorhandler(..).
    // So it is pretty delayed...
    CPPUNIT_ASSERT_LESS(2000ul, readMsg->size()); // Limit a bit to get aware if sometimes it takes longer...

    // Back to original value (no big harm if this line is not met due to previous exception).
    net::AmqpClient::m_maxMessageSize -= 1;
}

void Amqp_Test::testHashClient() {
    if (m_defaultBrokers.empty()) {
        std::clog << " No AMQP broker in environment. Skipping hash client tests..." << std::endl;
        return;
    }
    // The AmqpHashClient needs an EventLoop for the deserialisation
    std::jthread eventLoopThread([](std::stop_token stoken) { karabo::net::EventLoop::work(); });

    // Prepare connection - will get connected automatically once clients need that
    net::AmqpConnection::Pointer connection(std::make_shared<net::AmqpConnection>(m_defaultBrokers));

    const std::string prefix = net::Broker::brokerDomainFromEnv() += ".";

    // Create a bob, just for sending
    net::AmqpHashClient::HashReadHandler bobRead = [](const data::Hash::Pointer&, const data::Hash::Pointer&,
                                                      const std::string&, const std::string&) {};
    net::AmqpHashClient::ErrorReadHandler bobError = [](const std::string&) {};
    net::AmqpHashClient::Pointer bob(
          net::AmqpHashClient::create(connection, prefix + "bob", AMQP::Table(), bobRead, bobError));


    // Now create alice that subscribes and thus should receive
    data::Hash::Pointer readHeader = std::make_shared<data::Hash>();
    data::Hash::Pointer readBody = std::make_shared<data::Hash>();
    auto readExchange = std::make_shared<std::string>();
    auto readKey = std::make_shared<std::string>();
    auto readMutex = std::make_shared<std::mutex>();
    net::AmqpHashClient::HashReadHandler aliceRead = [readHeader, readBody, readExchange, readKey, readMutex](
                                                           const data::Hash::Pointer& h, const data::Hash::Pointer& b,
                                                           const std::string& exchange, const std::string& key) {
        std::scoped_lock lock(*readMutex);
        *readHeader = *h;
        *readBody = *b;
        *readExchange = exchange;
        *readKey = key;
    };
    auto readErrorNumber = std::make_shared<std::atomic<int>>(0);
    auto readErrorString = std::make_shared<std::string>();
    net::AmqpHashClient::ErrorReadHandler aliceError = [readErrorNumber, readErrorString](const std::string& msg) {
        *readErrorString = msg;
        ++(*readErrorNumber);
    };
    net::AmqpHashClient::Pointer alice(
          net::AmqpHashClient::create(connection, prefix + "alice", AMQP::Table(), aliceRead, aliceError));

    auto aliceSubDone = std::make_shared<std::promise<boost::system::error_code>>();
    auto aliceSubFut = aliceSubDone->get_future();
    alice->asyncSubscribe(prefix + "hashExchange", "alice",
                          [aliceSubDone](const boost::system::error_code ec) { aliceSubDone->set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, aliceSubFut.wait_for(m_timeout));
    const boost::system::error_code aliceSubEc = aliceSubFut.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(aliceSubEc.message(), static_cast<int>(boost::system::errc::success),
                                 aliceSubEc.value());

    // Now bob sends a message
    data::Hash::Pointer sentHeader = std::make_shared<data::Hash>("headerLine", "fromBob");
    data::Hash::Pointer sentBody = std::make_shared<data::Hash>("a1", "the answer is", "a2", 42);
    auto bobPubDone = std::make_shared<std::promise<boost::system::error_code>>();
    auto bobPubFut = bobPubDone->get_future();
    bob->asyncPublish(prefix + "hashExchange", "alice", sentHeader, sentBody,
                      [bobPubDone](const boost::system::error_code ec) { bobPubDone->set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, bobPubFut.wait_for(m_timeout));
    const boost::system::error_code bobPubEc = bobPubFut.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(bobPubEc.message(), static_cast<int>(boost::system::errc::success), bobPubEc.value());

    for (unsigned int i = 0; i < m_timeoutMs; ++i) {
        std::this_thread::sleep_for(1ms);
        std::scoped_lock lock(*readMutex);
        if (!readHeader->empty() && !readBody->empty()) break;
    }
    CPPUNIT_ASSERT(readHeader);
    CPPUNIT_ASSERT(readBody);
    CPPUNIT_ASSERT_EQUAL(prefix + "hashExchange", *readExchange);
    CPPUNIT_ASSERT_EQUAL(std::string("alice"), *readKey);

    CPPUNIT_ASSERT(readHeader->has("headerLine"));
    CPPUNIT_ASSERT_EQUAL(std::string("fromBob"), readHeader->get<std::string>("headerLine"));
    CPPUNIT_ASSERT_EQUAL(1ul, readHeader->size());

    CPPUNIT_ASSERT_EQUAL(2ul, readBody->size());
    CPPUNIT_ASSERT_EQUAL(std::string("the answer is"), readBody->get<std::string>("a1"));
    CPPUNIT_ASSERT_EQUAL(42, readBody->get<int>("a2"));

    // Test sending something that fails (e.g. cannot be deserialised)
    // Create a rawbob to send binary data - no need for a read handler
    net::AmqpClient::Pointer rawBob(std::make_shared<net::AmqpClient>(connection, prefix + "rawbob", AMQP::Table(),
                                                                      net::AmqpClient::ReadHandler()));
    CPPUNIT_ASSERT_EQUAL(0, readErrorNumber->load()); // no error yet
    rawBob->asyncPublish(prefix + "hashExchange", "alice", std::make_shared<std::vector<char>>(100, 'r'),
                         [](const boost::system::error_code ec) {});

    for (unsigned int i = 0; i < m_timeoutMs; ++i) {
        if (readErrorNumber->load() > 0) break;
        std::this_thread::sleep_for(1ms);
    }
    CPPUNIT_ASSERT_EQUAL(1, readErrorNumber->load());
    CPPUNIT_ASSERT_MESSAGE(*readErrorString, !readErrorString->empty()); // no matter what fails

    karabo::net::EventLoop::stop();
}
