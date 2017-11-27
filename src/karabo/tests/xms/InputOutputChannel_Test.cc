/* 
 * File:   InputOutputChannel_Test.cc
 * Author: flucke
 * 
 * Created on November 8, 2016, 3:54 PM
 */

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
        input->connect(outputInfo);
        boost::this_thread::sleep(boost::posix_time::milliseconds(100)); // time for TCP setup

        // Write data again (twice in one go...) - now input is connected.
        output->write(Hash("key", 43));
        output->write(Hash("key", -43));
        output->update();

        int trials = 20;
        while (--trials >= 0) {
            if (2u == calls) break;
            boost::this_thread::sleep(boost::posix_time::milliseconds(2)); // time for callback
        }
        CPPUNIT_ASSERT_EQUAL(2u, calls);

        // Disconnect
        input->disconnect(outputChannelId);
    }
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    // Write data again - input does not anymore receive data.
    output->write(Hash("key", 44));
    output->update();
    // Extended time for callback to be really sure nothing comes.
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    // still 2:
    CPPUNIT_ASSERT_EQUAL(2u, calls);

}

