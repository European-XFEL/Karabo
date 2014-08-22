/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <string>

#include "StateMachineTest.hh"

namespace exfel {
    namespace core {

        using namespace krb_log4cpp;

        using namespace std;
        using namespace exfel::util;

        void StateMachineTest::noStateTransition(const std::string& typeId, int state) {
            string eventName(typeId);
            boost::regex re(".*\\d+(.+Event).*");
            boost::smatch what;
            bool result = boost::regex_search(typeId, what, re);
            if (result && what.size() == 2) {
                eventName = what.str(1);
            }
            std::cout << "Current state does not allow any transition for event \"" << eventName << "\"";
        }
        
        void StateMachineTest::updateCurrentState(const std::string& currentState) {
            cout << "State Change: " << currentState << endl;
        }
        
        void StateMachineTest::errorStateOnEntry() {
            
        }
        
        void StateMachineTest::errorStateOnExit() {
            
        }
        
        void StateMachineTest::okOnEntry() {}
        
        void StateMachineTest::okOnExit() {}
        
        void StateMachineTest::a1OnEntry(){
            std::cout << "I still made it here!" << std::endl;
        }
        
        void StateMachineTest::a1OnExit(){}

        void StateMachineTest::aOnEntry() {
            
        }
        
        void StateMachineTest::aOnExit() {
            
        }
        
//        void StateMachineTest::bOnEntry() {
//            
//        }
//        
//        void StateMachineTest::bOnExit() {
//            
//        }
                
        void StateMachineTest::errorFoundAction(const std::string& , const std::string&) {
        
        }
        
        void StateMachineTest::a2A1Action() {
            throw PARAMETER_EXCEPTION("Alarm");
        }
        
        void StateMachineTest::a2BAction() {
            throw PARAMETER_EXCEPTION("Alarm");
        }
        
        void StateMachineTest::b2AAction(const int& i) {
            
        }
        
        void StateMachineTest::endErrorAction() {
            
        }
        
        bool StateMachineTest::goToAGuard(const int& i) {
            if (i == 1) return true;
            else return false;
        }
        
      
        
       


    } // namespace core
} // namespace exfel
