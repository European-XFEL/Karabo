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

            virtual ~BaseFsm() {
            }

            static void expectedParameters(const karabo::util::Schema&) {
            }

            virtual void initFsmSlots() {
            }

            virtual void exceptionFound(const std::string&, const std::string&) const = 0;

            KARABO_FSM_ON_EXCEPTION(exceptionFound);

            KARABO_DEPRECATED virtual void onStateUpdate(const std::string& currentState) = 0;

            virtual void updateState(const std::string& state) = 0;

            KARABO_FSM_ON_CURRENT_STATE_CHANGE(updateState);

            virtual void onNoStateTransition(const std::string& typeId, int state) = 0;

            KARABO_FSM_ON_NO_STATE_TRANSITION(onNoStateTransition);

            virtual void preStartFsm() {
            }

            virtual void startFsm() {
            }
            
            virtual void stopWorkers() {
            }

        };
    }
}
#endif
