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

#ifndef __CLASS_NAME_ALL_CAPS___HH
#define __CLASS_NAME_ALL_CAPS___HH

#include "karabo/core/Device.hh"
#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/Schema.hh"
#include "version.hh" // provides __PACKAGE_NAME_ALL_CAPS___PACKAGE_VERSION


namespace karabo {

    class __CLASS_NAME__ final : public karabo::core::Device {
       public:
        // Add reflection information and Karabo framework compatibility to
        // this class.
        KARABO_CLASSINFO(__CLASS_NAME__, "__CLASS_NAME__", __PACKAGE_NAME_ALL_CAPS___PACKAGE_VERSION)

        /**
         * @brief Necessary method as part of the factory/configuration system
         *
         * @param expected Will contain a description of expected parameters
         * for a device of this class.
         */
        static void expectedParameters(karabo::data::Schema& expected);

        /**
         * @brief Constructs a device with the initial configuration given by a
         * Hash.
         *
         * @param config the initial device configuration.
         *
         * If this class is constructed using the configuration system, the
         * Hash object will already have been validated using the resulting
         * schema of the expectedParameters function.
         */
        explicit __CLASS_NAME__(const karabo::data::Hash& config);

        /**
         * @brief Called in case the device gets killed.
         */
        virtual ~__CLASS_NAME__();

        /**
         * @brief Acts as a hook and is called after an reconfiguration request
         * was received, but BEFORE the reconfiguration request is actually
         * merged into this device's state.
         *
         * @param incomingReconfiguration The reconfiguration information as
         * triggered externally. You can change the content of this Hash before
         * it is merged into the device's current state.
         *
         * @note (a) The incomingReconfiguration was validated before
         *       (b) If you do not need to handle the reconfigured data, there
         *           is no need to implement this function. The reconfiguration
         *           will automatically be applied to the current state.
         *
         */
        virtual void preReconfigure(karabo::data::Hash& incomingReconfiguration) override;

        /**
         * @brief Acts as a hook and is called after an reconfiguration request
         * was received, and AFTER this reconfiguration request got merged into
         * this device's current state.
         *
         * @note You may access any (updated or not) parameters using the usual
         * getters and setters:
         *
         * @code
         * int i = get<int>("myParam");
         * @endcode
         */
        virtual void postReconfigure() override;

       private:
        void initialize();

    }; // class __CLASS_NAME__

} // namespace karabo

#endif
