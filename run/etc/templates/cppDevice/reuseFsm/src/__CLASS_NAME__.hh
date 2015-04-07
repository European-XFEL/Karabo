/*
 * $Id$
 *
 * Author: <__EMAIL__>
 * 
 * Created on __DATE__
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO___CLASS_NAME_ALL_CAPS___HH
#define KARABO___CLASS_NAME_ALL_CAPS___HH

#include <karabo/karabo.hpp>

/**
 * The main Karabo namespace
 */
namespace karabo {

    class __CLASS_NAME__ : public karabo::core::Device<karabo::core::StartStopFsm> {

    public:
        
        // Add reflection and version information to this class
        KARABO_CLASSINFO(__CLASS_NAME__, "__CLASS_NAME__", "1.0")

        /**
         * Necessary method as part of the factory/configuration system
         * @param expected Will contain a description of expected parameters for this device
         */
        static void expectedParameters(karabo::util::Schema& expected);

        /**
         * Constructor providing the initial configuration in form of a Hash object.
         * If this class is constructed using the configuration system the Hash object will
         * already be validated using the information of the expectedParameters function.
         * The configuration is provided in a key/value fashion. 
         */
        __CLASS_NAME__(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed (i.e. the event-loop returns)
         */
        virtual ~__CLASS_NAME__() {
            KARABO_LOG_INFO << "dead.";
        }

    private: // State-machine call-backs (override)

        void initializationStateOnEntry();

        void startAction();

        void stopAction();
    };
}

#endif
