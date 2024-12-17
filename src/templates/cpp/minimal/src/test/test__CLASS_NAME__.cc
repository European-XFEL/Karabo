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

#include <gtest/gtest.h>

#include <memory>
#include <thread>
#include <utility>

#include "__CLASS_NAME__.hh"
#include "karabo/util/Hash.hh"
#include "testrunner.hh"

#define TEST_DEVICE_ID "test__CLASS_NAME__"

using namespace ::testing;

/**
 * @brief Test fixture for the __CLASS_NAME__ device class.
 *        Any mandatory configuration for the device needs to be
 *        added here. Additionally, one can derive other test fixtures
 *        from this default one to create fixtures with different
 *        instantiation configurations or different mocking behaviour.
 */
class __CLASS_NAME__DefaultCfg : public KaraboDeviceFixture {
   protected:
    __CLASS_NAME__DefaultCfg() : KaraboDeviceFixture() {}

    void SetUp() override {
        /**
         * Add configuration for this 'DefaultCfg' test fixture
         * to the devCfg hash here
         */

        karabo::util::Hash devCfg("deviceId", TEST_DEVICE_ID, "_deviceId_", TEST_DEVICE_ID);

        /**
         * Instantiate device without device server so the device pointer
         * is returned and accessible for use with googlemock
         *
         * Because some features are not fully supported in this case, the device under
         * test will behave differently compared to one instantiated within a device server
         *
         * Known limitations of the unit test device
         *
         *   - It does not receive time ticks since the device server calls slotTimeTick directly
         *     (which is not exposed as a slot...)
         *   - onTimeTick(tracinId, seco, frac, period) will never get called
         *   - onTimeUpdate will never get called
         */
        // instantiate the device to be tested
        // karabo::core::BaseDevice::Pointer baseDevice;
        // baseDevice = instantiateAndGetPointer("__CLASS_NAME__", TEST_DEVICE_ID, devCfg);
        // cast the BaseDevice::Pointer to the derived class Pointer
        // deviceUnderTest = boost::dynamic_pointer_cast<karabo::__CLASS_NAME__>(baseDevice);

        /**
         * Instantiate device inside a device server
         *
         * Recommended method if not using googletest/googlemock expectations
         */
        // instantiate the device to be tested
        instantiateWithDeviceServer("__CLASS_NAME__", TEST_DEVICE_ID, devCfg);

        /**
         * Add default expectations for this test fixture here
         */
    }

    void TearDown() override {
        /**
         * Shutdown the device
         */
        // test the preDestruction() hook
        m_deviceCli->execute(TEST_DEVICE_ID, "slotKillDevice");
        // test device class destruction
        deviceUnderTest.reset();
    }

    karabo::__CLASS_NAME__::Pointer deviceUnderTest;
};


// test only that device instantiates
TEST_F(__CLASS_NAME__DefaultCfg, testDeviceInstantiation) {
    karabo::util::Hash result = m_deviceCli->get(TEST_DEVICE_ID);
    std::string cls = result.get<std::string>("classId");
    std::string clsVer = result.get<std::string>("classVersion");

    std::cout << std::endl;
    std::cout << "Device under test is class " << cls;
    std::cout << ", version " << clsVer;
    std::cout << std::endl;
    std::cout << std::endl;

    ASSERT_STREQ(cls.c_str(), "__CLASS_NAME__");
}
