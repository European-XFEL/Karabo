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

// #include "karabo/log/Logger.hh" // needed to switch on logging below
#include "karabo/net/AmqpClient2.hh"
#include "karabo/net/AmqpConnection.hh"
#include "karabo/net/Broker.hh"
#include "karabo/tests/BrokerUtils.hh"

using namespace karabo;

CPPUNIT_TEST_SUITE_REGISTRATION(Amqp_Test);

Amqp_Test::Amqp_Test()
    // Use the environment variable KARABO_CI_BROKERS to define the brokers to use in tests:
    // e.g. export KARABO_CI_BROKERS=tcp://a-jms-broker:7777;amqp://an-amqp-broker:5672
    // If that variable is not defined, KARABO_BROKER with an amqp broker also works
    : m_defaultBrokers(getBrokerFromEnv("amqp")) {
    // Uncomment this and the logger include for log output:
    // log::Logger::configure(util::Hash("priority", "DEBUG"));
    // log::Logger::useOstream();
}

Amqp_Test::~Amqp_Test() {}


void Amqp_Test::testConnection() {
    const std::chrono::seconds timeout(5);
    const std::string urlBadHostPort("amqp://wha:whu@invalidhost:5555");
    if (m_defaultBrokers.empty()) {
        std::clog << " No AMQP broker in environment. Skipping tests that require it..." << std::flush;
    } else {
        // test asyncConnect - proper url
        net::AmqpConnection::Pointer connection(boost::make_shared<net::AmqpConnection>(m_defaultBrokers));
        std::promise<boost::system::error_code> done;
        auto fut = done.get_future();
        connection->asyncConnect([&done](const boost::system::error_code ec) { done.set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut.wait_for(timeout));
        const boost::system::error_code ec = fut.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), static_cast<int>(boost::system::errc::success), ec.value());
        // We can safely destruct the connection again.
        // But note: use_count() maybe > 1 (if handler being called), then destruction happens elsewhere)
        CPPUNIT_ASSERT_NO_THROW(connection.reset());

        // test asyncConnect - but improper user/password url
        std::string urlBadUser = m_defaultBrokers.front();
        const size_t endUserPasswd = urlBadUser.find('@'); // 7 is size of "amqp://"
        boost::replace_first(urlBadUser, urlBadUser.substr(7, endUserPasswd - 7), "invalid:user_password");
        connection = boost::make_shared<net::AmqpConnection>(std::vector<std::string>(1, urlBadUser));
        std::promise<boost::system::error_code> done2;
        auto fut2 = done2.get_future();
        connection->asyncConnect([&done2](const boost::system::error_code ec) { done2.set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut2.wait_for(timeout));
        const boost::system::error_code ec2 = fut2.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec2.message(), static_cast<int>(boost::system::errc::connection_refused),
                                     ec2.value());
        CPPUNIT_ASSERT_NO_THROW(connection.reset());

        // test asyncConnect with > 1 urls, but all bad: will receive error code matching the last url
        // (first: last is urlBadHostPort)
        std::vector<std::string> urls({urlBadUser, urlBadHostPort});
        connection = boost::make_shared<net::AmqpConnection>(urls);
        std::promise<boost::system::error_code> done4;
        auto fut4 = done4.get_future();
        connection->asyncConnect([&done4](const boost::system::error_code ec) { done4.set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut4.wait_for(timeout));
        const boost::system::error_code ec4 = fut4.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec4.message(), // not_connected since last url is urlBadHostPort
                                     static_cast<int>(boost::system::errc::not_connected), ec4.value());
        CPPUNIT_ASSERT_NO_THROW(connection.reset());

        // (first: last is bad credentials)
        urls = {urlBadHostPort, urlBadUser};
        connection = boost::make_shared<net::AmqpConnection>(urls);
        std::promise<boost::system::error_code> done5;
        auto fut5 = done5.get_future();
        connection->asyncConnect([&done5](const boost::system::error_code ec) { done5.set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut5.wait_for(timeout));
        const boost::system::error_code ec5 = fut5.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec5.message(), // connection_refused since last url is 'urlBadUser'
                                     static_cast<int>(boost::system::errc::connection_refused), ec5.value());
        CPPUNIT_ASSERT_NO_THROW(connection.reset());

        // Now test 3 addresses, last one valid
        urls.push_back(m_defaultBrokers.front());
        connection = boost::make_shared<net::AmqpConnection>(urls);
        std::promise<boost::system::error_code> done3;
        auto fut3 = done3.get_future();
        connection->asyncConnect([&done3](const boost::system::error_code ec) { done3.set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut3.wait_for(3 * timeout));
        const boost::system::error_code ec3 = fut3.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec3.message(), static_cast<int>(boost::system::errc::success), ec3.value());

        // Here add test for successful channel creation
        std::promise<std::shared_ptr<AMQP::Channel>> doneCreation;
        auto futCreateChannel = doneCreation.get_future();
        connection->asyncCreateChannel(
              [&doneCreation](const std::shared_ptr<AMQP::Channel>& channel, const char* errMsg) {
                  if (errMsg) doneCreation.set_value(nullptr);
                  else doneCreation.set_value(channel);
              });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futCreateChannel.wait_for(std::chrono::seconds(5)));
        auto channel = futCreateChannel.get();
        CPPUNIT_ASSERT(channel); // a channel has been created
        CPPUNIT_ASSERT_NO_THROW(channel.reset());

        CPPUNIT_ASSERT_NO_THROW(connection.reset());
    }

    { // test invalid tcp address - the tests for post, dispatch and too early asyncCreateChannel sneaked in as well
        const std::vector<std::string> invalidIps(1, urlBadHostPort);
        net::AmqpConnection::Pointer connection(boost::make_shared<net::AmqpConnection>(invalidIps));

        // first test post(..) and dispatch(..)
        std::promise<void> donePost;
        auto futPost = donePost.get_future();
        connection->post([&donePost]() { donePost.set_value(); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futPost.wait_for(timeout));
        futPost.get();

        std::promise<void> doneDispatch;
        auto futDispatch = doneDispatch.get_future();
        connection->dispatch([&doneDispatch]() { doneDispatch.set_value(); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futDispatch.wait_for(timeout));
        futDispatch.get();
        // TODO? Add a test that checks that dispatching a method means directly calling it if already in the
        // io_context?

        // then test failing channel creation without being connected
        std::promise<std::string> doneCreation;
        auto futCreateChannel = doneCreation.get_future();

        connection->asyncCreateChannel(
              [&doneCreation](const std::shared_ptr<AMQP::Channel>& channel, const char* errMsg) {
                  if (channel) doneCreation.set_value("Non empty channelPtr!");
                  else doneCreation.set_value(errMsg);
              });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futCreateChannel.wait_for(std::chrono::seconds(5)));
        CPPUNIT_ASSERT_EQUAL(std::string("Connection not ready"), futCreateChannel.get());

        // Now the real test for invalid tcp address
        std::promise<boost::system::error_code> done;
        auto fut = done.get_future();
        connection->asyncConnect([&done](const boost::system::error_code ec) { done.set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut.wait_for(std::chrono::seconds(5)));
        const boost::system::error_code ec = fut.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), static_cast<int>(boost::system::errc::not_connected), ec.value());

        // We can safely destruct the connection again
        CPPUNIT_ASSERT_NO_THROW(connection.reset());
    }
    { // test wrongly formatted address
        net::AmqpConnection::Pointer connection(
              boost::make_shared<net::AmqpConnection>(std::vector<std::string>(1, "not://proper:protocol")));
        std::promise<boost::system::error_code> done;
        auto fut = done.get_future();
        connection->asyncConnect([&done](const boost::system::error_code ec) { done.set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut.wait_for(std::chrono::seconds(5)));
        const boost::system::error_code ec = fut.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), static_cast<int>(boost::system::errc::wrong_protocol_type),
                                     ec.value());
        CPPUNIT_ASSERT_NO_THROW(connection.reset());
    }
}

void Amqp_Test::testClient() {
    if (m_defaultBrokers.empty()) {
        std::clog << " No AMQP broker in environment. Skipping client tests..." << std::endl;
        return;
    }
    const std::chrono::seconds timeout(5);

    // Prepare valid connection
    net::AmqpConnection::Pointer connection(boost::make_shared<net::AmqpConnection>(m_defaultBrokers));
    std::promise<boost::system::error_code> done;
    auto fut = done.get_future();
    connection->asyncConnect([&done](const boost::system::error_code ec) { done.set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut.wait_for(timeout));
    const boost::system::error_code ec = fut.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), static_cast<int>(boost::system::errc::success), ec.value());

    // Create client "bob" with a read handler that simply appends data it reads to a container ('readByBob')
    std::vector<std::tuple<std::shared_ptr<std::vector<char>>, std::string, std::string>> readByBob;
    std::atomic<int> readByBobCounter(0);
    std::promise<void> bobRead4;
    auto futBobRead4 = bobRead4.get_future();
    auto readHandlerBob = [&readByBob, &readByBobCounter, &bobRead4](const std::shared_ptr<std::vector<char>>& data,
                                                                     const std::string& exchange,
                                                                     const std::string& routingKey) {
        readByBob.push_back({data, exchange, routingKey});
        if (++readByBobCounter == 4) {
            bobRead4.set_value();
        }
    };
    // To avoid interference of different test runs, any exchange and queue (i.e. nominal client instanceId) are
    // prefixed with broker domain (i.e. Karabo topic)
    const std::string prefix = net::Broker::brokerDomainFromEnv() += ".";
    net::AmqpClient2::Pointer bob(
          boost::make_shared<net::AmqpClient2>(connection, prefix + "bob", AMQP::Table(), readHandlerBob));

    // In parallel subscribe twice while channel is created under the hood
    std::promise<boost::system::error_code> subDone1;
    auto fut1 = subDone1.get_future();
    bob->asyncSubscribe(prefix + "exchange", "bob1",
                        [&subDone1](const boost::system::error_code ec) { subDone1.set_value(ec); });
    std::promise<boost::system::error_code> subDone2;
    auto fut2 = subDone2.get_future();
    bob->asyncSubscribe(prefix + "exchange", "bob2",
                        [&subDone2](const boost::system::error_code ec) { subDone2.set_value(ec); });

    // Now wait for both subscriptions to be done
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut1.wait_for(timeout));
    const boost::system::error_code ec1 = fut1.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec1.message(), static_cast<int>(boost::system::errc::success), ec1.value());

    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut2.wait_for(timeout));
    const boost::system::error_code ec2 = fut2.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec2.message(), static_cast<int>(boost::system::errc::success), ec2.value());

    // Subscribe twice more after channel is already created
    std::promise<boost::system::error_code> subDone3;
    auto fut3 = subDone3.get_future();
    bob->asyncSubscribe(prefix + "exchange", "bob3",
                        [&subDone3](const boost::system::error_code ec) { subDone3.set_value(ec); });
    std::promise<boost::system::error_code> subDone4;
    auto fut4 = subDone4.get_future();
    bob->asyncSubscribe(prefix + "exchange", "bob4",
                        [&subDone4](const boost::system::error_code ec) { subDone4.set_value(ec); });

    // Again wait for both subscriptions to be done
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut3.wait_for(timeout));
    const boost::system::error_code ec3 = fut3.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec3.message(), static_cast<int>(boost::system::errc::success), ec3.value());

    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut4.wait_for(timeout));
    const boost::system::error_code ec4 = fut4.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec4.message(), static_cast<int>(boost::system::errc::success), ec4.value());

    // Now create 2nd client 'alice' and let it talk to 'bob' - no need to subscribe beforehand
    // Note that 'alice' will only one message at the very end.
    std::atomic<int> numReadAlice(0);
    auto readHandlerAlice = [&numReadAlice](const std::shared_ptr<std::vector<char>>& data, const std::string& exchange,
                                            const std::string& routingKey) { ++numReadAlice; };
    net::AmqpClient2::Pointer alice(
          boost::make_shared<net::AmqpClient2>(connection, prefix + "alice", AMQP::Table(), readHandlerAlice));

    std::vector<std::future<boost::system::error_code>> publishFutures;
    for (size_t i = 0; i < 5; ++i) {
        auto publishDone = std::make_shared<std::promise<boost::system::error_code>>();
        publishFutures.push_back(publishDone->get_future());
        std::string routingKey("bob ");
        routingKey.back() = '1' + i; // "bob1", "bob2", ..., "bob5" - note that bob did not subscribe to "bob5"!
        alice->asyncPublish(prefix + "exchange", routingKey, std::make_shared<std::vector<char>>(10, 'a' + i),
                            [publishDone{std::move(publishDone)}](const boost::system::error_code ec) {
                                publishDone->set_value(ec);
                            });
    }
    // Wait for confirmations of all published messages
    for (std::future<boost::system::error_code>& fut : publishFutures) {
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut.wait_for(timeout));
        const boost::system::error_code ec = fut.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), static_cast<int>(boost::system::errc::success), ec.value());
    }

    // Bob should have received the first four messages (and in order),
    // but not the fifth since bob did not subscribe to routingKey "bob5"
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futBobRead4.wait_for(timeout));
    CPPUNIT_ASSERT_EQUAL(4, readByBobCounter.load());
    CPPUNIT_ASSERT_EQUAL(4ul, readByBob.size());
    for (size_t i = 0; i < readByBob.size(); ++i) {
        // All on same exchange, but with different routing keys
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Message " + karabo::util::toString(i), prefix + "exchange",
                                     std::get<1>(readByBob[i]));
        std::string routingKey("bob ");
        routingKey.back() = '1' + i; // "bob1", "bob2", "bob3", "bob4"
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Message " + karabo::util::toString(i), routingKey, std::get<2>(readByBob[i]));
        // Check data content
        const std::shared_ptr<std::vector<char>>& data = std::get<0>(readByBob[i]);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Message " + karabo::util::toString(i), 10ul, data->size());
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Message " + karabo::util::toString(i), static_cast<char>('a' + i), (*data)[0]);
    }

    // Give some time for the fifth message - though it should not come
    boost::this_thread::sleep(boost::posix_time::milliseconds(300));
    CPPUNIT_ASSERT_EQUAL(4ul, readByBob.size());

    // Now test alice subscribing and bob publishing - it has different order between subcription and publish than bob
    std::promise<boost::system::error_code> subDoneAlice;
    auto futAlice = subDoneAlice.get_future();
    alice->asyncSubscribe(prefix + "other_exchange", "alice",
                          [&subDoneAlice](const boost::system::error_code ec) { subDoneAlice.set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futAlice.wait_for(timeout));
    const boost::system::error_code ecAlice = futAlice.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecAlice.message(), static_cast<int>(boost::system::errc::success), ecAlice.value());

    // Simply check that a message arrives after max. 2 seconds...
    CPPUNIT_ASSERT_EQUAL(0, numReadAlice.load());
    bob->asyncPublish(prefix + "other_exchange", "alice", std::make_shared<std::vector<char>>(5, 'b'),
                      [](const boost::system::error_code ec) {});
    for (int i = 0; i < 1000; ++i) {
        if (numReadAlice >= 1) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(2));
    }
    CPPUNIT_ASSERT_EQUAL(1, numReadAlice.load());
}
