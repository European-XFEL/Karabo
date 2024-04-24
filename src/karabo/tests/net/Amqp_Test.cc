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

#include "karabo/log/Logger.hh"
#include "karabo/net/AmqpClient2.hh"
#include "karabo/net/AmqpConnection.hh"
#include "karabo/net/Broker.hh"
#include "karabo/tests/BrokerUtils.hh"
#include "karabo/util/StringTools.hh"

using namespace karabo;

CPPUNIT_TEST_SUITE_REGISTRATION(Amqp_Test);

Amqp_Test::Amqp_Test()
    // Use the environment variable KARABO_CI_BROKERS to define the brokers to use in tests:
    // e.g. export KARABO_CI_BROKERS=tcp://a-jms-broker:7777;amqp://an-amqp-broker:5672
    // If that variable is not defined, KARABO_BROKER with an amqp broker also works
    : m_defaultBrokers(getBrokerFromEnv("amqp")), m_timeout(120) {
    if (false &&                            // remove 'false &&' to switch on logging
        !m_loggingIsSetup.exchange(true)) { // configure logging only once in the process
        log::Logger::configure(util::Hash("priority", "DEBUG"));
        log::Logger::useOstream();
    }
}

Amqp_Test::~Amqp_Test() {}


void Amqp_Test::testConnection() {
    const std::string urlBadHostPort("amqp://wha:whu@invalidhost:5555");
    if (m_defaultBrokers.empty()) {
        std::clog << " No AMQP broker in environment. Skipping those connection tests that require it..." << std::flush;
    } else {
        // test asyncConnect - proper url
        net::AmqpConnection::Pointer connection(boost::make_shared<net::AmqpConnection>(m_defaultBrokers));
        std::promise<boost::system::error_code> done;
        auto fut = done.get_future();
        connection->asyncConnect([&done](const boost::system::error_code ec) { done.set_value(ec); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut.wait_for(m_timeout));
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
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut2.wait_for(m_timeout));
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
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut4.wait_for(m_timeout));
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
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut5.wait_for(m_timeout));
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
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut3.wait_for(3 * m_timeout));
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
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futPost.wait_for(m_timeout));
        futPost.get();

        std::promise<void> doneDispatch;
        auto futDispatch = doneDispatch.get_future();
        connection->dispatch([&doneDispatch]() { doneDispatch.set_value(); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futDispatch.wait_for(m_timeout));
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

    // Prepare valid connection
    net::AmqpConnection::Pointer connection(boost::make_shared<net::AmqpConnection>(m_defaultBrokers));
    std::promise<boost::system::error_code> done;
    auto fut = done.get_future();
    connection->asyncConnect([&done](const boost::system::error_code ec) { done.set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut.wait_for(m_timeout));
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
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut1.wait_for(m_timeout));
    const boost::system::error_code ec1 = fut1.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec1.message(), static_cast<int>(boost::system::errc::success), ec1.value());

    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut2.wait_for(m_timeout));
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
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut3.wait_for(m_timeout));
    const boost::system::error_code ec3 = fut3.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec3.message(), static_cast<int>(boost::system::errc::success), ec3.value());

    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut4.wait_for(m_timeout));
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
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut.wait_for(m_timeout));
        const boost::system::error_code ec = fut.get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), static_cast<int>(boost::system::errc::success), ec.value());
    }

    // Bob should have received the first four messages (and in order),
    // but not the fifth since bob did not subscribe to routingKey "bob5"
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futBobRead4.wait_for(m_timeout));
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

    //***************************************************************
    // Now test alice subscribing and bob publishing - it has different order between subcription and publish than bob
    std::promise<boost::system::error_code> subDoneAlice;
    auto futAlice = subDoneAlice.get_future();
    alice->asyncSubscribe(prefix + "other_exchange", "alice",
                          [&subDoneAlice](const boost::system::error_code ec) { subDoneAlice.set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futAlice.wait_for(m_timeout));
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

    //***************************************************************
    // Now test unsubscribing
    std::promise<boost::system::error_code> unsubDoneAlice;
    auto futUnsubAlice = unsubDoneAlice.get_future();
    alice->asyncUnsubscribe(prefix + "other_exchange", "alice",
                            [&unsubDoneAlice](const boost::system::error_code ec) { unsubDoneAlice.set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futUnsubAlice.wait_for(m_timeout));
    const boost::system::error_code ecAliceUnsub = futUnsubAlice.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecAliceUnsub.message(), static_cast<int>(boost::system::errc::success),
                                 ecAliceUnsub.value());

    //***************************************************************
    // Test that, after alice has unsubscribed above, it does not receive further messages
    std::promise<boost::system::error_code> writeDoneBob;
    auto futWriteBob = writeDoneBob.get_future();
    bob->asyncPublish(prefix + "other_exchange", "alice", std::make_shared<std::vector<char>>(4, 'c'),
                      [&writeDoneBob](const boost::system::error_code ec) { writeDoneBob.set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futWriteBob.wait_for(m_timeout));
    const boost::system::error_code ecBobWrite = futWriteBob.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecBobWrite.message(), static_cast<int>(boost::system::errc::success),
                                 ecBobWrite.value());

    boost::this_thread::sleep(boost::posix_time::milliseconds(300)); // Grant some message travel time...
    CPPUNIT_ASSERT_EQUAL(1, numReadAlice.load());                    // ...but nothing arrives due to unsubscription!

    //***************************************************************
    // Test unsubscription of something not subscribed - gives success (though that is debatable)
    std::promise<boost::system::error_code> unsubDoneAlice2;
    auto futUnsubAlice2 = unsubDoneAlice2.get_future();
    alice->asyncUnsubscribe(prefix + "other_exchange", "not_subscribed_routing_key",
                            [&unsubDoneAlice2](const boost::system::error_code ec) { unsubDoneAlice2.set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futUnsubAlice2.wait_for(m_timeout));
    const boost::system::error_code ecAliceUnsub2 = futUnsubAlice2.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecAliceUnsub2.message(), static_cast<int>(boost::system::errc::success),
                                 ecAliceUnsub2.value());

    //***************************************************************
    // Little concurrency test: Subscribing and immediately unsubscribing work
    std::promise<boost::system::error_code> subDoneAlice2;
    auto futSubAlice2 = subDoneAlice2.get_future();
    std::promise<boost::system::error_code> unsubDoneAlice3;
    auto futUnsubAlice3 = unsubDoneAlice3.get_future();
    alice->asyncSubscribe(prefix + "other_exchange", "alice",
                          [&subDoneAlice2](const boost::system::error_code ec) { subDoneAlice2.set_value(ec); });
    alice->asyncUnsubscribe(prefix + "other_exchange", "alice",
                            [&unsubDoneAlice3](const boost::system::error_code ec) { unsubDoneAlice3.set_value(ec); });

    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futSubAlice2.wait_for(m_timeout));
    const boost::system::error_code ecSubAlice2 = futSubAlice2.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecSubAlice2.message(), static_cast<int>(boost::system::errc::success),
                                 ecSubAlice2.value());
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futUnsubAlice3.wait_for(m_timeout));
    const boost::system::error_code ecAliceUnsub3 = futUnsubAlice3.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecAliceUnsub3.message(), static_cast<int>(boost::system::errc::success),
                                 ecAliceUnsub3.value());
    // And still, alice does not receive Bob's message
    std::promise<boost::system::error_code> writeDoneBob2;
    auto futWriteBob2 = writeDoneBob2.get_future();
    bob->asyncPublish(prefix + "other_exchange", "alice", std::make_shared<std::vector<char>>(4, 'd'),
                      [&writeDoneBob2](const boost::system::error_code ec) { writeDoneBob2.set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futWriteBob2.wait_for(m_timeout));
    const boost::system::error_code ecBobWrite2 = futWriteBob2.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ecBobWrite2.message(), static_cast<int>(boost::system::errc::success),
                                 ecBobWrite2.value());

    boost::this_thread::sleep(boost::posix_time::milliseconds(300)); // Grant some message travel time...
    CPPUNIT_ASSERT_EQUAL(1, numReadAlice.load());                    // ...but nothing arrives due to unsubscription!
}

void Amqp_Test::testClientSameId() {
    if (m_defaultBrokers.empty()) {
        std::clog << " No AMQP broker in environment. Skipping client tests for same id..." << std::endl;
        return;
    }
    // Prepare valid connection
    net::AmqpConnection::Pointer connection(boost::make_shared<net::AmqpConnection>(m_defaultBrokers));
    std::promise<boost::system::error_code> done;
    auto fut = done.get_future();
    connection->asyncConnect([&done](const boost::system::error_code ec) { done.set_value(ec); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, fut.wait_for(m_timeout));
    const boost::system::error_code ec = fut.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), static_cast<int>(boost::system::errc::success), ec.value());

    // To avoid interference of different test runs, any exchange and queue (i.e. nominal client instanceId) are
    // prefixed with broker domain (i.e. Karabo topic)
    const std::string prefix = net::Broker::brokerDomainFromEnv() += ".";


    // Create first client with id "bob"
    std::vector<std::tuple<std::shared_ptr<std::vector<char>>, std::string, std::string>> readByBob;
    std::promise<std::shared_ptr<std::vector<char>>> bobReadDone;
    auto bobReadFut = bobReadDone.get_future();
    auto readHandlerBob = [&bobReadDone](const std::shared_ptr<std::vector<char>>& data, const std::string& exchange,
                                         const std::string& routingKey) { bobReadDone.set_value(data); };
    net::AmqpClient2::Pointer bob(
          boost::make_shared<net::AmqpClient2>(connection, prefix + "bob", AMQP::Table(), readHandlerBob));

    std::promise<boost::system::error_code> bobSubDone;
    auto bobSubFut = bobSubDone.get_future();
    bob->asyncSubscribe(prefix + "exchange", "bob",
                        [&bobSubDone](const boost::system::error_code ec) { bobSubDone.set_value(ec); });
    const boost::system::error_code bobSubEc = bobSubFut.get(); // now directly get(), tired of m_timeout
    CPPUNIT_ASSERT_EQUAL_MESSAGE(bobSubEc.message(), static_cast<int>(boost::system::errc::success), bobSubEc.value());

    //***************************************************************
    // Create another client again with id "bob"
    std::vector<std::tuple<std::shared_ptr<std::vector<char>>, std::string, std::string>> readByBob2;
    std::promise<std::shared_ptr<std::vector<char>>> bob2ReadDone;
    auto bob2ReadFut = bob2ReadDone.get_future();
    auto readHandlerBob2 = [&bob2ReadDone](const std::shared_ptr<std::vector<char>>& data, const std::string& exchange,
                                           const std::string& routingKey) { bob2ReadDone.set_value(data); };
    net::AmqpClient2::Pointer bob2( // Same id (prefix + "bob") as above!
          boost::make_shared<net::AmqpClient2>(connection, prefix + "bob", AMQP::Table(), readHandlerBob2));

    std::promise<boost::system::error_code> bob2SubDone;
    auto bob2SubFut = bob2SubDone.get_future();
    bob2->asyncSubscribe(prefix + "exchange", "bob", // subscribe to same as other bob
                         [&bob2SubDone](const boost::system::error_code ec) { bob2SubDone.set_value(ec); });
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
    std::vector<net::AmqpClient2::Pointer> clients;
    std::vector<std::atomic<int>> receivedFlags(numClients);
    for (int i = 0; i < numClients; ++i) {
        auto readHandler = [i, &receivedFlags](const std::shared_ptr<std::vector<char>>& data,
                                               const std::string& exchange,
                                               const std::string& routingKey) { ++(receivedFlags[i]); };
        clients.push_back(
              boost::make_shared<net::AmqpClient2>(connection, prefix + "alice", AMQP::Table(), readHandler));
    }

    // Now let them all subscribe (and thus create channel, queue and consumer) in parallel
    std::vector<std::promise<boost::system::error_code>> subPromises;
    std::vector<std::future<boost::system::error_code>> subFutures;
    for (int i = 0; i < numClients; ++i) {
        subPromises.push_back(std::promise<boost::system::error_code>());
        subFutures.push_back(subPromises.back().get_future());
        clients[i]->asyncSubscribe(
              prefix + "exchange", "alice", // subscribe to same for all
              [i, &subPromises](const boost::system::error_code ec) { subPromises[i].set_value(ec); });
    }
    // Block until all confirm subscription
    for (int i = 0; i < numClients; ++i) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Future of client " + karabo::util::toString(i), std::future_status::ready,
                                     subFutures[i].wait_for(m_timeout));
        const boost::system::error_code ec = subFutures[i].get();
        CPPUNIT_ASSERT_EQUAL_MESSAGE((karabo::util::toString(i) += ": ") + ec.message(),
                                     static_cast<int>(boost::system::errc::success), ec.value());
    }

    // Now send a message and verify its arrival at all clients
    clients[0]->asyncPublish(prefix + "exchange", "alice", std::make_shared<std::vector<char>>(4, 'a'),
                             [](const boost::system::error_code ec) {});
    bool allReceivedOne = false;
    for (int i = 0; i < 2000; ++i) {
        allReceivedOne = true;
        for (int iClient = 0; iClient < numClients; ++iClient) {
            const int nReceived = receivedFlags[iClient];
            CPPUNIT_ASSERT_LESSEQUAL(1, nReceived); // receiving twice is a bug
            if (nReceived != 1) {
                allReceivedOne = false;
                break; // no need to check other clients
            }
        }
        if (allReceivedOne) break;
        else boost::this_thread::sleep(boost::posix_time::milliseconds(1));
    }
    CPPUNIT_ASSERT(allReceivedOne);
}
