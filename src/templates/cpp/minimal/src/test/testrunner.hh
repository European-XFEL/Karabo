/*
 * Author: __EMAIL__
 *
 * Created on __DATE__
 * from template '__TEMPLATE_ID__' of Karabo __KARABO_VERSION__
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef TESTRUNNER_HH
#define TESTRUNNER_HH

#include <thread>
#include <utility>
#include <gtest/gtest.h>

#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/PluginLoader.hh"

#define DEVICE_SERVER_ID "testDeviceSrvCpp"
#define LOG_PRIORITY     "FATAL"  // Can also be "DEBUG", "INFO" or "ERROR"

#define DEV_CLI_TIMEOUT_SEC 2


/**
 * @brief Test fixture for setup/teardown of karabo event loop,
 *        device server, and device client.
 * 
 *        This class is generic and does not need to be modified
 *        to support different device classes.
 */
class KaraboDeviceFixture: public testing::Test {
protected:

    KaraboDeviceFixture();
    ~KaraboDeviceFixture();

    // instantiate an instance of the classID (with the devCfg configuration hash)
    // and return the BaseDevice::Pointer for that instance
    karabo::core::BaseDevice::Pointer instantiateAndGetPointer(
        const std::string& classId,
        const std::string& instanceId,
        const karabo::util::Hash& devCfg);
    // create a device server and ask it to instantiate the classID (with the devCfg
    // configuration hash)
    void instantiateWithDeviceServer(
        const std::string& classId,
        const std::string& instanceId,
        const karabo::util::Hash& devCfg);

    std::thread m_eventLoopThread;
    karabo::core::DeviceServer::Pointer m_deviceSrv;
    karabo::core::DeviceClient::Pointer m_deviceCli;
};

#endif
