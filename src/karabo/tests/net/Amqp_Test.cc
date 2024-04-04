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
#include "karabo/net/AmqpConnection.hh"
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
        std::promise<std::shared_ptr<AMQP::TcpChannel>> doneCreation;
        auto futCreateChannel = doneCreation.get_future();
        connection->asyncCreateChannel(
              [&doneCreation](const std::shared_ptr<AMQP::TcpChannel>& channel, const char* errMsg) {
                  if (errMsg) doneCreation.set_value(nullptr);
                  else doneCreation.set_value(channel);
              });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futCreateChannel.wait_for(std::chrono::seconds(5)));
        auto channel = futCreateChannel.get();
        CPPUNIT_ASSERT(channel); // a channel has been created
        CPPUNIT_ASSERT_NO_THROW(channel.reset());

        CPPUNIT_ASSERT_NO_THROW(connection.reset());
    }

    { // test invalid tcp address - the tests for post and too early asyncCreateChannel sneaked in as well
        const std::vector<std::string> invalidIps(1, urlBadHostPort);
        net::AmqpConnection::Pointer connection(boost::make_shared<net::AmqpConnection>(invalidIps));

        // first test post(..)
        std::promise<void> donePost;
        auto futPost = donePost.get_future();
        connection->post([&donePost]() { donePost.set_value(); });
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, futPost.wait_for(timeout));
        futPost.get();

        // then test failing channel creation without being connected
        std::promise<std::string> doneCreation;
        auto futCreateChannel = doneCreation.get_future();

        connection->asyncCreateChannel(
              [&doneCreation](const std::shared_ptr<AMQP::TcpChannel>& channel, const char* errMsg) {
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
