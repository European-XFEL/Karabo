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


#ifndef EXFEL_XMS_INPUT_HH
#define	EXFEL_XMS_INPUT_HH

#include "AbstractInput.hh"

namespace exfel {
    namespace xms {

        template <class T>
        class Input : public AbstractInput {
        public:

            EXFEL_CLASSINFO(Input, "Input", "1.0")
            EXFEL_FACTORY_BASE_CLASS

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(exfel::util::Schema& expected) {
                
                AbstractInput::expectedParameters(expected);
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const exfel::util::Hash & input) {
            }
            
       
            virtual void read(T& data, size_t idx = 0) = 0;
            
            virtual size_t size() const = 0;
            
        };
        
        typedef exfel::xms::Input<exfel::util::Hash > HashInput;
        typedef exfel::xms::Input<std::string> FileWrapInput;
    }
}



#endif	/* EXFEL_PACKAGENAME_INPUT_HH */

