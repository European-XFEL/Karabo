/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "DemoDevice1.hh"

using namespace std;
using namespace exfel::util;
using namespace log4cpp;

namespace exfel {
    namespace core {

        EXFEL_REGISTER_FACTORY_2_CC(Device, ReconfigurableFsm, DemoDevice1)


        DemoDevice1::~DemoDevice1() {
        }

        void DemoDevice1::expectedParameters(Schema& expected) {

            STRING_ELEMENT(expected).key("firstWord")
                    .displayedName("First Word")
                    .description("Input for first word")
                    .assignmentOptional().defaultValue("")
                    .unitName("dimensionless")
                    .unitSymbol("")
                    .reconfigurable()
                    .commit();
            
            STRING_ELEMENT(expected).key("secondWord")
                    .displayedName("Second Word")
                    .description("Input for first word")
                    .assignmentOptional().defaultValue("")
                    .unitName("dimensionless")
                    .unitSymbol("")
                    .reconfigurable()
                    .commit();
            
            INT32_ELEMENT(expected).key("multiply")
                    .displayedName("Multiply")
                    .description("multiplies word")
                    .assignmentOptional().defaultValue(1)
                    .unitName("#")
                    .unitSymbol("#")
                    .allowedStates("ErrorState")
                    .reconfigurable()
                    .commit();

            STRING_ELEMENT(expected).key("composedWord")
                    .displayedName("Composed word")
                    .description("The composed word")
                    .assignmentOptional().noDefaultValue()
                    .unitName("dimensionless")
                    .unitSymbol("")
                    .readOnly()
                    .commit();
        }

        void DemoDevice1::configure(const Hash& input) {
            
        }

        
        void DemoDevice1::onReconfigure(exfel::util::Hash& incomingReconfiguration) {
            // Current state
            string firstWord = get<string>("firstWord");
            string secondWord = get<string>("secondWord");
            int multiply = get<int>("multiply");
            
            // Changes
            if(incomingReconfiguration.has("firstWord")) incomingReconfiguration.get("firstWord", firstWord);
            if(incomingReconfiguration.has("secondWord")) incomingReconfiguration.get("secondWord", secondWord);
            if(incomingReconfiguration.has("multiply")) {
                multiply = incomingReconfiguration.get<int>("multiply");
            }
            throw PARAMETER_EXCEPTION("ALAARM!!");
            // Update a read-only variable
            string composedWord = firstWord + " " + secondWord;
            for(int i = 0; i < multiply; ++i) {
                composedWord += " " + composedWord;
            }
            set("composedWord", multiply);
        }
    }
}
