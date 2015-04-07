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

#include <beckhoff/BeckhoffDevice.hh>

/**
 * The main Karabo namespace
 */
namespace karabo {

    class __CLASS_NAME__ : public karabo::beckhoff::BeckhoffDevice {

    public:

        // Add reflection and compatibility information to this class
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
         * The destructor will be called in case the device gets shut-down (i.e. the event-loop returns)
         */
        virtual ~__CLASS_NAME__();
       
        
    private: 
        
        // This function will send the command "start" to the PLC
        void start();
        
        // This function will send the command "stop" to the PLC
        void stop();
        
        /**
         * Function that maps bits into a string. 
         * The bits encode the status of the Beckhoff PLC.
         * The function will be called-back whenever the PLC sends a new status update.
         * 
         * @param hardwareStatusBitField The bits as send by the PLC
         * @return The state (as string)
         * 
         */
        std::string decodeHardwareState(const std::bitset<32>& hardwareStatusBitField);
        
        /**
         * This function allows to update the software state according to a given hardware state.         
         * The function is called-back after decoding of the PLC bits happened (see above).
         * Overriding using this function is optional, in case you do not override, the above decoded state will
         * be taken 'as is' as the software state.
         */
        void onHardwareStatusUpdate(const std::string& hwState);

    };
}

#endif
