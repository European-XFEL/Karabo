/* 
 * File:   FsmBase.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 10, 2013, 12:14 PM
 */

#ifndef KARABO_CORE_FSMBASE_HH_hh
#define	KARABO_CORE_FSMBASE_HH_hh

#include <karabo/core/FsmMacros.hh>
#include <karabo/util/karaboDll.hh>

namespace karabo {
    namespace core {
        
        
        class FsmBase : public virtual karabo::xms::SignalSlotable {
            
        public:
            
            static void expectedParameters(const karabo::util::Schema&) {}
            
            virtual void initFsmSlots() {}
            
            virtual void errorFound(const std::string&, const std::string&) = 0;
            
            virtual void errorFoundAction(const std::string&, const std::string&) = 0;
            
            virtual void onStateUpdate(const std::string& currentState) = 0;
            
            virtual void startFsm() = 0;
            
            KARABO_FSM_LOGGER(DEBUG); // TODO Clean up to use new logging system
            
            KARABO_FSM_ON_EXCEPTION(errorFound);
            
            KARABO_FSM_NO_TRANSITION_PV_ACTION(noStateTransition);
            
            KARABO_FSM_ON_CURRENT_STATE_CHANGE(onStateUpdate);

        };
    }
}
#endif
