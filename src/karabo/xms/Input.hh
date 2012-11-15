/*
 * $Id$
 *
 * File:   Input.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:22 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XMS_INPUT_HH
#define	KARABO_XMS_INPUT_HH

#include "AbstractInput.hh"

namespace karabo {
    namespace xms {

        template <class T>
        class Input : public AbstractInput {
        public:

            KARABO_CLASSINFO(Input, "Input", "1.0")
            KARABO_FACTORY_BASE_CLASS

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
                
                //AbstractInput::expectedParameters(expected);
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const karabo::util::Hash & input) {
            }
            
       
            virtual void read(T& data, size_t idx = 0) = 0;
            
            virtual size_t size() const = 0;
            
        };
        
        typedef karabo::xms::Input<karabo::util::Hash > HashInput;
        typedef karabo::xms::Input<std::string> FileWrapInput;
    }
}



#endif	/* KARABO_PACKAGENAME_INPUT_HH */

