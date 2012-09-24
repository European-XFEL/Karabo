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


#ifndef EXFEL_XMS_OUTPUT_HH
#define	EXFEL_XMS_OUTPUT_HH

#include <karabo/util/Factory.hh>

#include "AbstractOutput.hh"

namespace exfel {

    namespace xms {

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
        
        
        typedef exfel::xms::Output<exfel::util::Hash > HashOutput;
        typedef exfel::xms::Output<std::string> FileWrapOutput;
    }
}

#endif	

