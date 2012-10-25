/*
 * $Id$
 *
 * File:   Output.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 29, 2011, 9:44 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_XIP_OUTPUT_HH
#define	EXFEL_XIP_OUTPUT_HH

#include <exfel/util/Factory.hh>

#include "AbstractOutput.hh"

namespace exfel {

    namespace xip {

        template <class T>
        class Output : public AbstractOutput {
        public:

            EXFEL_CLASSINFO(Output, "Output", "1.0")
            
            EXFEL_FACTORY_BASE_CLASS

            Output() {
            }

            virtual ~Output() {
            }
            
            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(exfel::util::Schema& expected) {
                
                AbstractOutput::expectedParameters(expected);
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const exfel::util::Hash & input) {
            }
            
            virtual void write(const T& object) = 0;
        };
        
        
        typedef exfel::xip::Output<exfel::util::Hash > HashOutput;
        typedef exfel::xip::Output<std::string> FileWrapOutput;
    }
}

#endif	

