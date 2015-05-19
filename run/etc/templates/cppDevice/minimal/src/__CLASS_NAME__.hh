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

    class __CLASS_NAME__ : public karabo::core::Device<> {

    public:

        // Add reflection information and Karabo framework compatibility to this class
        KARABO_CLASSINFO(__CLASS_NAME__, "__CLASS_NAME__", "1.3")

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
        virtual ~__CLASS_NAME__();
       
        /**
         * This function acts as a hook and is called after an reconfiguration request was received,
         * but BEFORE this reconfiguration request is actually merged into this device's state.
         * 
         * The reconfiguration information is contained in the Hash object provided as an argument.
         * You have a chance to change the content of this Hash before it is merged into the device's current state.
         * 
         * NOTE: (a) The incomingReconfiguration was validated before
         *       (b) If you do not need to handle the reconfigured data, there is no need to implement this function.
         *           The reconfiguration will automatically be applied to the current state.
         * @param incomingReconfiguration The reconfiguration information as was triggered externally
         */
        virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration);
        
        
        /**
         * This function acts as a hook and is called after an reconfiguration request was received,
         * and AFTER this reconfiguration request got merged into this device's current state.
         * You may access any (updated or not) parameters using the usual getters and setters.
         * @code
         * int i = get<int>("myParam");
         * @endcode
         */
        virtual void postReconfigure();
         

    private:

    };
}

#endif
