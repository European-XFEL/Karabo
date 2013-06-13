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

        class BaseFsm : public virtual karabo::xms::SignalSlotable {

        public:

            static void expectedParameters(const karabo::util::Schema&) {
            }

            virtual void initFsmSlots() {
            }

            virtual void errorFound(const std::string&, const std::string&) = 0;
            
            KARABO_FSM_ON_EXCEPTION(errorFound);

            virtual void errorFoundAction(const std::string&, const std::string&) = 0;

            virtual void onStateUpdate(const std::string& currentState) = 0;
            
            KARABO_FSM_ON_CURRENT_STATE_CHANGE(onStateUpdate);
            
            virtual void onNoStateTransition(const std::string& typeId, int state) = 0;
            
            KARABO_FSM_ON_NO_STATE_TRANSITION(onNoStateTransition);

            virtual void startFsm() = 0;

        };
    }
}
#endif
