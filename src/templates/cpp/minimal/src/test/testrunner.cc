/*
 * Author: __EMAIL__
 *
 * Created on __DATE__
 * from template '__TEMPLATE_ID__' of Karabo __KARABO_VERSION__
 *
 * This file is intended to be used together with Karabo:
 *
 * http://www.karabo.eu
 *
 * IF YOU REQUIRE ANY LICENSING AND COPYRIGHT TERMS, PLEASE ADD THEM HERE.
 * Karabo itself is licensed under the terms of the MPL 2.0 license.
 */

#include "testrunner.hh"


KaraboDeviceFixture::KaraboDeviceFixture() {
    m_eventLoopThread = std::jthread(&karabo::net::EventLoop::work);

    // Instantiate C++ Device Client
    m_deviceCli = karabo::core::DeviceClient::MakeShared(std::string(), false);
    m_deviceCli->initialize();
}

void KaraboDeviceFixture::instantiateWithDeviceServer(const std::string& classId, const std::string& instanceId,
                                                      const karabo::data::Hash& devCfg) {
    // Instantiate C++ device server
    const karabo::data::Hash& pluginConfig = karabo::data::Hash("pluginDirectory", ".");
    karabo::util::PluginLoader::create("PluginLoader", pluginConfig)->update();

    // scanPlugins is set to true to scan $KARABO/plugins directory
    // can be set to false if other libraries are not needed for testing
    karabo::data::Hash config("serverId", DEVICE_SERVER_ID, "log.level", LOG_PRIORITY);
    m_deviceSrv = karabo::core::DeviceServer::create("DeviceServer", config);
    m_deviceSrv->finalizeInternalInitialization();

    // instantiate the device under test
    std::pair<bool, std::string> success =
          m_deviceCli->instantiate(DEVICE_SERVER_ID, classId, devCfg, DEV_CLI_TIMEOUT_SEC);

    ASSERT_TRUE(success.first) << "Failure instantiating '" << instanceId << "':\n" << success.second;
}

karabo::core::Device::Pointer KaraboDeviceFixture::instantiateAndGetPointer(const std::string& classId,
                                                                            const std::string& instanceId,
                                                                            const karabo::data::Hash& devCfg) {
    std::string errorMsg;
    karabo::core::Device::Pointer devPtr;
    // karabo::log::Logger is a singleton and we should reset it to make sure it is configured how we like it.
    karabo::log::Logger::reset();
    karabo::data::Hash config("level", LOG_PRIORITY);
    karabo::log::Logger::configure(config);
    karabo::log::Logger::useConsole();

    try {
        // instantiate the device under test
        devPtr = karabo::core::Device::create(classId, devCfg);
        // build a broker configuration Hash
        using namespace karabo::net;
        using namespace karabo::data;
        const std::string brokerType = Broker::brokerTypeFromEnv();
        // connect the device under test to the broker
        Broker::Pointer connection = Broker::create(brokerType, Hash("instanceId", instanceId));
        devPtr->finalizeInternalInitialization(
              connection,
              true, // no server feeds the device with broadcasts, so it has to listen itself
              "");  // timeserver id (only needed by slotGetTime) does not matter
    } catch (const std::exception& e) {
        errorMsg = e.what();
        if (errorMsg.empty()) errorMsg = "Unexpected instantiation exception";
    }

    EXPECT_TRUE(errorMsg.empty()) << "Failure instantiating '" << instanceId << "':" << std::endl
                                  << errorMsg << std::endl;

    return devPtr;
}

KaraboDeviceFixture::~KaraboDeviceFixture() {
    m_deviceCli.reset();
    m_deviceSrv.reset();
    karabo::net::EventLoop::stop();
    m_eventLoopThread.join();
}


/*
 * @brief GoogleTest entry point
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
