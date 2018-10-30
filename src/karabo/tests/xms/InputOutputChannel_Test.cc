/* 
 * File:   InputOutputChannel_Test.cc
 * Author: flucke
 * 
 * Created on November 8, 2016, 3:54 PM
 */

#include <future>

#include "InputOutputChannel_Test.hh"

#include "karabo/xms/InputChannel.hh"
#include "karabo/xms/OutputChannel.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/util/Hash.hh"
#include "karabo/net/EventLoop.hh"

using namespace karabo;
using xms::InputChannel;
using xms::OutputChannel;
using util::Configurator;
using util::Hash;

CPPUNIT_TEST_SUITE_REGISTRATION(InputOutputChannel_Test);


InputOutputChannel_Test::InputOutputChannel_Test() {
}


void InputOutputChannel_Test::setUp() {
    // Event loop started in xmsTestRunner.cc's main().
}


void InputOutputChannel_Test::tearDown() {
}


void InputOutputChannel_Test::testConnectDisconnect() {

    // Setup input channel
    InputChannel::Pointer input = Configurator<InputChannel>::create("InputChannel", Hash());
    input->setInstanceId("inputChannel");
    unsigned int calls = 0;
    input->registerDataHandler([&calls](const Hash& data, const InputChannel::MetaData & meta) {
        ++calls;
    });

    // Setup output channel
    OutputChannel::Pointer output = Configurator<OutputChannel>::create("OutputChannel", Hash());
    output->setInstanceIdAndName("outputChannel", "output");

    // Write first data - nobody connected yet.
    output->write(Hash("key", 42));
    output->update();
    boost::this_thread::sleep(boost::posix_time::milliseconds(20)); // time for call back
    CPPUNIT_ASSERT_EQUAL(0u, calls);

    // Connect
    const std::string outputChannelId("outputChannelString");
    Hash outputInfo(output->getInformation());
    outputInfo.set("outputChannelString", outputChannelId);
    outputInfo.set("memoryLocation", "local");
    size_t n = 200;
    for (size_t i = 0; i < n; ++i) {
        calls = 0;
        // Setup connection handler
        std::promise<karabo::net::ErrorCode> connectErrorCode;
        auto connectFuture = connectErrorCode.get_future();
        auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode & ec) {
            connectErrorCode.set_value(ec);
        };
        // initiate connect and block until done (fail test if timeout))
        input->connect(outputInfo, connectHandler);
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, connectFuture.wait_for(std::chrono::milliseconds(500)));
        CPPUNIT_ASSERT_EQUAL(connectFuture.get(), karabo::net::ErrorCode()); // i.e. no error

        // Now ensure that output channel took note of input registration:
        bool connected = false;
        int trials = 200;
        do {
            // By default, InputChannel is configured to receive a "copy" of all data and not to share with others
            if (output->hasRegisteredCopyInputChannel(input->getInstanceId())) {
              connected = true;
              break;
            }
            boost::this_thread::sleep(boost::posix_time::milliseconds(2));
        } while (--trials >= 0);
        CPPUNIT_ASSERT(connected);

        // Write data again (twice in one go...) - now input is connected.
        output->write(Hash("key", 43));
        output->write(Hash("key", -43));
        output->update();

        trials = 200;
        while (--trials >= 0) {
            if (2u == calls) break;
            boost::this_thread::sleep(boost::posix_time::milliseconds(2)); // time for callback
        }
        CPPUNIT_ASSERT_EQUAL(2u, calls);

        // Disconnect
        input->disconnect(outputChannelId);
        // Some time to travel for message
        trials = 200;
        connected = true;
        do {
            if (!output->hasRegisteredCopyInputChannel(input->getInstanceId())) {
                connected = false;
                break;
            }
            boost::this_thread::sleep(boost::posix_time::milliseconds(2));
        } while (--trials >= 0);
        CPPUNIT_ASSERT(!connected);
    }

    // Write data again - input does not anymore receive data.
    output->write(Hash("key", 44));
    output->update();
    // Extended time for callback to be really sure nothing comes.
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    // still 2:
    CPPUNIT_ASSERT_EQUAL(2u, calls);

    // Now test connection attempts that fail
    std::vector<karabo::util::Hash> badOutputInfos;
    // Not supported protocol (only tcp works):
    badOutputInfos.push_back(outputInfo);
    badOutputInfos.back().set("connectionType", "udp");
    // Invalid port to connect to (client needs a specific one):
    badOutputInfos.push_back(outputInfo);
    badOutputInfos.back().set("port", 0u);
    // Non-existing host:
    badOutputInfos.push_back(outputInfo);
    badOutputInfos.back().set("hostname", "exflblablupp-not-there.desy.de");

    for (const Hash& badOutputInfo : badOutputInfos) {
        // Setup connection handler
        std::promise<karabo::net::ErrorCode> connectErrorCode;
        auto connectFuture = connectErrorCode.get_future();
        auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode & ec) {
            connectErrorCode.set_value(ec);
        };
        input->connect(badOutputInfo, connectHandler);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Connection handler not called in time for " + toString(badOutputInfo),
                                     std::future_status::ready, connectFuture.wait_for(std::chrono::milliseconds(1000)));
        CPPUNIT_ASSERT_MESSAGE("Connection did not fail for " + toString(badOutputInfo),
                               connectFuture.get() != karabo::net::ErrorCode()); // not all OK (do not care which problem)
    }
}

