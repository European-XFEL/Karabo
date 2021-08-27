/*
 * Author: __EMAIL__
 *
 * Created on __DATE__
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "__CLASS_NAME__.hh"

#include <boost/shared_ptr.hpp>
#include <gtest/gtest.h>
#include <thread>
#include <utility>

#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/PluginLoader.hh"


#define DEVICE_SERVER_ID "testDeviceSrvCpp"
#define TEST_DEVICE_ID   "test__CLASS_NAME__"
#define LOG_PRIORITY     "FATAL"  // Can also be "DEBUG", "INFO" or "ERROR"

#define DEV_CLI_TIMEOUT_SEC 2


/**
 * @brief Test fixture for the __CLASS_NAME__ device class.
 */
class __CLASS_NAME__Fixture: public testing::Test {
protected:

    __CLASS_NAME__Fixture() = default;

    void SetUp( ) {
        m_eventLoopThread = std::thread(&karabo::net::EventLoop::work);

        // Load the library dynamically
        const karabo::util::Hash& pluginConfig = karabo::util::Hash("pluginDirectory", ".");
        karabo::util::PluginLoader::create("PluginLoader", pluginConfig)->update();

        // Instantiate C++ Device Server.
        karabo::util::Hash config("serverId", DEVICE_SERVER_ID,
                                  "scanPlugins", true,
                                  "Logger.priority", LOG_PRIORITY);
        m_deviceSrv = karabo::core::DeviceServer::create("DeviceServer", config);
        m_deviceSrv->finalizeInternalInitialization();
        // Instantiate Device Client.
        m_deviceCli = boost::make_shared<karabo::core::DeviceClient>();
    }

    void TearDown( ) {
        m_deviceCli.reset();
        m_deviceSrv.reset();
        karabo::net::EventLoop::stop();
        m_eventLoopThread.join();
    }

    void instantiateTestDevice(const karabo::util::Hash& devSpecificCfg) {
        karabo::util::Hash devCfg("deviceId", TEST_DEVICE_ID);
        devCfg.merge(devSpecificCfg);

        std::pair<bool, std::string> success =
            m_deviceCli->instantiate(DEVICE_SERVER_ID, "__CLASS_NAME__",
                                     devCfg, DEV_CLI_TIMEOUT_SEC);

        ASSERT_TRUE(success.first)
            << "Error instantiating '" << TEST_DEVICE_ID << "':\n"
            << success.second;
    }

    void deinstantiateTestDevice() {
        ASSERT_NO_THROW(
            m_deviceCli->killDevice(TEST_DEVICE_ID, DEV_CLI_TIMEOUT_SEC))
        << "Failed to deinstantiate device '" << TEST_DEVICE_ID << "'";
    }

    std::thread m_eventLoopThread;

    karabo::core::DeviceServer::Pointer m_deviceSrv;
    karabo::core::DeviceClient::Pointer m_deviceCli;
};


// TODO: Give the test case a proper name (not "testScaffold")
TEST_F(__CLASS_NAME__Fixture, testScaffold){

    // TODO: Provide a non-empty config for the device under test.
    instantiateTestDevice(karabo::util::Hash());

    // TODO: Define a test body.

    deinstantiateTestDevice();
}
