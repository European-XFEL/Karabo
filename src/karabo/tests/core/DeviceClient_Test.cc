/* 
 * File:   DevcieClient_Test.cc
 * Author: flucke
 * 
 * Created on August 24, 2017, 9:49 AM
 */

#include <string>
#include <karabo/core/DeviceClient.hh>

#include "DeviceClient_Test.hh"

#include "karabo/util/Hash.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/NDArray.hh"
#include "karabo/xms/InputChannel.hh"
#include "karabo/xms/ImageData.hh"

#define KRB_TEST_MAX_TIMEOUT 5

CPPUNIT_TEST_SUITE_REGISTRATION(DeviceClient_Test);

using namespace karabo::core;
using namespace karabo::util;
using karabo::xms::InputChannel;
using karabo::xms::ImageData;


template<class Container>
void assertIgnoringOrder(const Container& expected, const Container& actual, const std::string& which) {
    CPPUNIT_ASSERT_EQUAL_MESSAGE(which, expected.size(), actual.size());

    for (auto itExpected = expected.cbegin(); itExpected != expected.cend(); ++itExpected) {
        CPPUNIT_ASSERT_MESSAGE(which + "." + *itExpected,
                               std::find(actual.cbegin(), actual.cend(), *itExpected) != actual.cend());
    }
}

DeviceClient_Test::DeviceClient_Test() {
}


DeviceClient_Test::~DeviceClient_Test() {
}


void DeviceClient_Test::setUp() {
    // uncomment this if ever testing against a local broker
    // setenv("KARABO_BROKER", "tcp://localhost:7777", true);

    // Event loop is started in coreTestRunner.cc's main()

    const Hash config("serverId", "testServerDeviceClient", "scanPlugins", false, "Logger.priority", "FATAL");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();

    // Create client
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());

}


void DeviceClient_Test::tearDown() {
    m_deviceClient.reset();
    m_deviceServer.reset();
}


void DeviceClient_Test::testAll() {
    // A single test to reduce setup/teardown time
    testGet();
    testMonitorChannel();
    testGetSchema();
    testGetSchemaNoWait();
    testConnectionHandling();
}


void DeviceClient_Test::testGet() {

    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerDeviceClient", "PropertyTest",
                                                                       Hash("deviceId", "TestedDevice"),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // int as normal type - test both get, i.e. return by reference argument or return by value
    int intProperty = 0;
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get<int>("TestedDevice", "int32Property", intProperty));
    CPPUNIT_ASSERT_EQUAL(32000000, intProperty);
    intProperty = 0;
    CPPUNIT_ASSERT_NO_THROW(intProperty = m_deviceClient->get<int>("TestedDevice", "int32Property"));
    CPPUNIT_ASSERT_EQUAL(32000000, intProperty);

    // State is specially treated: Internally it is a string, but that should stay internal!
    State state(State::UNKNOWN);
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get<State>("TestedDevice", "state", state));
    CPPUNIT_ASSERT(state != State::UNKNOWN); // still INIT or very likely NORMAL
    state = State::UNKNOWN;
    CPPUNIT_ASSERT_NO_THROW(state = m_deviceClient->get<State>("TestedDevice", "state"));
    CPPUNIT_ASSERT(state != State::UNKNOWN);
    //
    std::string dummy;
    CPPUNIT_ASSERT_THROW(m_deviceClient->get<std::string>("TestedDevice", "state", dummy), ParameterException);
    CPPUNIT_ASSERT_THROW(dummy = m_deviceClient->get<std::string>("TestedDevice", "state"), ParameterException);

    // The same for AlarmConditionState
    AlarmCondition alarm(AlarmCondition::ALARM);
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get<AlarmCondition>("TestedDevice", "alarmCondition", alarm));
    CPPUNIT_ASSERT(alarm == AlarmCondition::NONE); // no alarm on device!
    alarm = AlarmCondition::ALARM;
    CPPUNIT_ASSERT_NO_THROW(alarm = m_deviceClient->get<AlarmCondition>("TestedDevice", "alarmCondition"));
    CPPUNIT_ASSERT(alarm == AlarmCondition::NONE); // no alarm on device!
    //
    CPPUNIT_ASSERT_THROW(m_deviceClient->get<std::string>("TestedDevice", "alarmCondition", dummy), ParameterException);
    CPPUNIT_ASSERT_THROW(dummy = m_deviceClient->get<std::string>("TestedDevice", "alarmCondition"), ParameterException);

    success = m_deviceClient->killDevice("TestedDevice", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
}


void DeviceClient_Test::testMonitorChannel() {
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerDeviceClient", "PropertyTest",
                                                                       Hash("deviceId", "TestedDevice2"),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Cannot unregister if nothing is registered
    CPPUNIT_ASSERT(!m_deviceClient->unregisterChannelMonitor("TestedDevice2", "output")); // existing channel
    CPPUNIT_ASSERT(!m_deviceClient->unregisterChannelMonitor("TestedDevice2", "notExistingoutput")); // non-existing channel
    CPPUNIT_ASSERT(!m_deviceClient->unregisterChannelMonitor("nonExistingDevice", "output")); // non-existing device

    // register data handler
    int int32inChannel = -1;
    std::string strInChannel("not a number");
    std::vector<long long> vecInt64inChannel;
    Dims ndArrayDims;
    float ndArrayEntry = -1.f;
    Dims imageDims;
    unsigned short imageEntry = 0;
    auto dataHandler = [&] (const Hash& data, const InputChannel::MetaData& metaData) {
        data.get("node.int32", int32inChannel);
        data.get("node.string", strInChannel);
        data.get("node.vecInt64", vecInt64inChannel);
        const NDArray& ndArray = data.get<NDArray>("node.ndarray");
        ndArrayDims = ndArray.getShape();
        if (ndArray.size() > 0) {
            ndArrayEntry = ndArray.getData<float>()[0];
        }
        const ImageData& image = data.get<ImageData>("node.image");
        imageDims = image.getDimensions();
        imageEntry = image.getData().getData<unsigned short>()[0];
    };
    CPPUNIT_ASSERT(m_deviceClient->registerChannelMonitor("TestedDevice2", "output", dataHandler));
    // not allowed to register again for same channel
    CPPUNIT_ASSERT(!m_deviceClient->registerChannelMonitor("TestedDevice2", "output", dataHandler));

    // Sleep a bit to ensure that input channel is connected (no callback to wait for... :-()
    boost::this_thread::sleep(boost::posix_time::milliseconds(50)); // TODO: How to make dynamic?
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestedDevice2", "writeOutput", KRB_TEST_MAX_TIMEOUT));

    int counter = 0;
    while (counter++ < 100) {
        if (int32inChannel == 1) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }
    // Now check all data arrived as it should:
    CPPUNIT_ASSERT_EQUAL(1, int32inChannel);
    CPPUNIT_ASSERT_EQUAL(std::string("1"), strInChannel);
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (100u), vecInt64inChannel.size());
    CPPUNIT_ASSERT_EQUAL(1ll, vecInt64inChannel[0]);
    CPPUNIT_ASSERT(ndArrayDims == Dims(100ull, 200ull));
    // Float comparison can fail, see e.g. https://git.xfel.eu/gitlab/Karabo/Framework/-/jobs/26996
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1., ndArrayEntry, 1.e-7);
    CPPUNIT_ASSERT(imageDims == Dims(400ull, 500ull));
    CPPUNIT_ASSERT_EQUAL(static_cast<unsigned short> (1), imageEntry);

    // unregister and trigger channel again
    CPPUNIT_ASSERT(m_deviceClient->unregisterChannelMonitor("TestedDevice2", "output"));
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestedDevice2", "writeOutput", KRB_TEST_MAX_TIMEOUT));
    // Give some time to any data that would travel - though there is none... Any way around this sleep?
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    // Since writing output is not monitored, int32inChannel stays as it is, i.e. we miss the '2'
    CPPUNIT_ASSERT_EQUAL(1, int32inChannel);

    // Register again and trigger channel once more
    CPPUNIT_ASSERT(m_deviceClient->registerChannelMonitor("TestedDevice2", "output", dataHandler));
    boost::this_thread::sleep(boost::posix_time::milliseconds(50)); // TODO: How to make dynamic?
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestedDevice2", "writeOutput", KRB_TEST_MAX_TIMEOUT));

    // Now should get the next number, i.e. 3
    counter = 0;
    while (counter++ < 100) {
        if (int32inChannel == 3) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }
    CPPUNIT_ASSERT_EQUAL(3, int32inChannel);

    // Now kill and re-instantiate sender device - should automatically reconnect.
    success = m_deviceClient->killDevice("TestedDevice2", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    success = m_deviceClient->instantiate("testServerDeviceClient", "PropertyTest",
                                          Hash("deviceId", "TestedDevice2"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    int32inChannel = -1;
    counter = 0;
    while (counter++ < 100) {
        if (int32inChannel > 0) break; // see comment below
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestedDevice2", "writeOutput", KRB_TEST_MAX_TIMEOUT));
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }
    // Do not care about the exact value of int32inChannel since auto-reconnect might take time
    CPPUNIT_ASSERT(int32inChannel > 0);
    CPPUNIT_ASSERT(int32inChannel <= counter);

    // Final clean-up
    success = m_deviceClient->killDevice("TestedDevice2", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
}


void DeviceClient_Test::testGetSchema() {

    // NOTE:
    // The deviceId needs to be another one than in the other tests, otherwise the test might succeed
    // even if the DeviceClient does not trigger to connect to schema updates: The registration that is
    // triggered by DeviceClient::get in 'testGet()' could still be valid.
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerDeviceClient", "PropertyTest",
                                                                       Hash("deviceId", "TestedDevice3"),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Check initial maxSize of one exemplary vector
    Schema schema(m_deviceClient->getDeviceSchema("TestedDevice3"));
    CPPUNIT_ASSERT(schema.hasMaxSize("vectors.floatProperty"));
    CPPUNIT_ASSERT_EQUAL(10u, schema.getMaxSize("vectors.floatProperty"));

    // Now update maxSize - this should trigger the signaling of an updated Schema and the client
    // should be informed since it should be "connected".
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestedDevice3", "slotUpdateSchema", KRB_TEST_MAX_TIMEOUT));

    // Wait a bit until new schema arrived
    unsigned int counter = 0;
    while (counter++ < 100) {
        schema = m_deviceClient->getDeviceSchema("TestedDevice3");
        if (schema.has("vectors.floatProperty") && schema.getMaxSize("vectors.floatProperty") == 20u) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }
    CPPUNIT_ASSERT(schema.hasMaxSize("vectors.floatProperty"));
    CPPUNIT_ASSERT_EQUAL(20u, schema.getMaxSize("vectors.floatProperty"));

    // Final clean-up
    success = m_deviceClient->killDevice("TestedDevice3", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
}


void DeviceClient_Test::testGetSchemaNoWait() {
    // NOTE: Better use new id, see comment in testGetSchema.
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerDeviceClient", "PropertyTest",
                                                                       Hash("deviceId", "TestedDevice4"),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    Schema schema(m_deviceClient->getDeviceSchemaNoWait("TestedDevice4"));
    // noWait and first request: nothing cached yet, so still empty
    CPPUNIT_ASSERT(schema.empty());

    // Wait a bit until schema arrived
    unsigned int counter = 0;
    while (counter++ < 100) {
        schema = m_deviceClient->getDeviceSchemaNoWait("TestedDevice4");
        if (!schema.empty()) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }

    // Check initial maxSize of one exemplary vector
    CPPUNIT_ASSERT(schema.hasMaxSize("vectors.floatProperty"));
    CPPUNIT_ASSERT_EQUAL(10u, schema.getMaxSize("vectors.floatProperty"));

    // FIXME: The test below to check for receiving schema updates is fragile and needs some sleep here to be reliable.
    //        The reason for the potential failure is a general ordering problem that we have in C++/bound Py:
    //        Currently (Nov 27, 2017), each call to getDeviceSchemaNoWait above triggered a call to the DeviceClient's
    //        "_slotSchemaUpdated". Also the following call to "slotUpdateSchema" will result in a call to
    //        "_slotSchemaUpdated", now with the updated schema. Unfortunately, this new schema might overtake one
    //        of the older ones. Then the correctly updated cache of the DeviceClient will be overwritten again by
    //        the delayed outdated schema...
    //        So this sleep here takes care that all schema requests that have been answered with the old schema
    //        have arrived at their destination before the change of the schema is triggered. :-(
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    // Now update maxSize - this should trigger the signaling of an updated Schema and the client
    // should be informed since it should be "connected".
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestedDevice4", "slotUpdateSchema", KRB_TEST_MAX_TIMEOUT));

    // Wait a bit until new schema arrived
    counter = 0;
    while (counter++ < 100) {
        schema = m_deviceClient->getDeviceSchemaNoWait("TestedDevice4");
        if (schema.has("vectors.floatProperty") && schema.getMaxSize("vectors.floatProperty") == 20u) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }
    CPPUNIT_ASSERT(schema.hasMaxSize("vectors.floatProperty"));
    CPPUNIT_ASSERT_EQUAL(20u, schema.getMaxSize("vectors.floatProperty"));

    // Final clean-up
    success = m_deviceClient->killDevice("TestedDevice4", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

}


void DeviceClient_Test::testConnectionHandling() {
    const std::string serverId("testServerDeviceClient");
    const std::string devId("TestedDevice");
    std::pair<bool, std::string> success = m_deviceClient->instantiate(serverId, "PropertyTest",
                                                                       Hash("deviceId", devId),
                                                                       KRB_TEST_MAX_TIMEOUT);
    ////////////////////////////////////////////////////////////////////
    // Test 1)
    // We check that we can get the configuration and a single property
    ////////////////////////////////////////////////////////////////////
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    CPPUNIT_ASSERT_EQUAL(32000000, m_deviceClient->get<int>(devId, "int32Property"));
    // Store all paths to cross check later full configurations:
    std::vector<std::string> paths;
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(devId).getPaths(paths));
    const std::vector<std::string> allPaths(paths);

    ////////////////////////////////////////////////////////////////////
    // Test 2)
    // We kill and restart the device and redo the test to check caching
    // and cache cleaning
    ////////////////////////////////////////////////////////////////////
    success = m_deviceClient->killDevice(devId, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Device not there, so timeout
    CPPUNIT_ASSERT_THROW(m_deviceClient->get(devId), karabo::util::TimeoutException);
    success = m_deviceClient->instantiate(serverId, "PropertyTest",
                                          Hash("deviceId", devId, "int32Property", 64000000),
                                          KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    CPPUNIT_ASSERT_EQUAL(64000000, m_deviceClient->get<int>(devId, "int32Property"));

    // Now check that still all paths are there
    paths.clear();
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(devId).getPaths(paths));
    assertIgnoringOrder(allPaths, paths, "killRestart");

    ////////////////////////////////////////////////////////////////////
    // Test 3)
    // We check whether the same as in test 2 works also in case a
    // monitor was registered which makes the device an immortal within
    // the device client and - after the device is shut down, even a
    // "zombie".
    ////////////////////////////////////////////////////////////////////
    bool cfgArrived = false;
    auto deviceMonitor = [&cfgArrived](const std::string&, const karabo::util::Hash&) {
        cfgArrived = true;
    };
    m_deviceClient->registerDeviceMonitor(devId, deviceMonitor);
    // TODO: Waiting should not be needed. For the very likely reason I need it here,
    // see DeviceClient::_slotChanged and DeviceClient::killDevice.
    int trials = 100;
    while (--trials >= 0) {
        if (cfgArrived) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(2));
    }
    // Kill the device again - within client it shall be a zombie now
    success = m_deviceClient->killDevice(devId, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Device not there again, so timeout
    CPPUNIT_ASSERT_THROW(m_deviceClient->get(devId), karabo::util::TimeoutException);

    // Restart device again with a changed property
    success = m_deviceClient->instantiate(serverId, "PropertyTest",
                                          Hash("deviceId", devId, "int32Property", -32000000),
                                          KRB_TEST_MAX_TIMEOUT);
    // Check again all paths and the single property
    Hash config;
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(devId, config));
    paths.clear();
    config.getPaths(paths);
    assertIgnoringOrder(allPaths, paths, "zombie");
    CPPUNIT_ASSERT_EQUAL(-32000000, config.get<int>("int32Property"));

    ////////////////////////////////////////////////////////////////////
    // Test 4)
    // Similar test content - but this time unregister the monitor to
    // get rid of the zombie before re-instantiation and checking again
    ////////////////////////////////////////////////////////////////////
    success = m_deviceClient->killDevice(devId, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    // Now device state in client is "zombie"

    // stop monitoring, i.e. kill zombie
    m_deviceClient->unregisterDeviceMonitor(devId);

    // Restart device again with a changed property
    success = m_deviceClient->instantiate(serverId, "PropertyTest",
                                          Hash("deviceId", devId, "int32Property", -64000000),
                                          KRB_TEST_MAX_TIMEOUT);

    // Check once more all paths and the single property
    config.clear();
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(devId, config));
    paths.clear();
    config.getPaths(paths);
    assertIgnoringOrder(allPaths, paths, "killedZombie");
    CPPUNIT_ASSERT_EQUAL(-64000000, config.get<int>("int32Property"));
}
