/*
 * $Id$
 *
 * Design, concepts and ideas: <serguei.essenov@xfel.eu>
 * Implementation: <burkhard.heisen@xfel.eu>
 *
 * Created on March 23, 2011, 1:28 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_DEVICESERVERLOGIC_HH
#define	KARABO_CORE_DEVICESERVERLOGIC_HH

#include "StateMachine.hh"

namespace karabo {

    namespace core {

        /**
         * The StateMachine class.
         */
        class DeviceServerLogic : public StateMachine {

        public:

            KARABO_CLASSINFO("DeviceServerStateMachine");

            DeviceServerLogic() {
            }

            void startStateMachine();

            /**************************************************************/
            /*                        EventSlots                          */
            /**************************************************************/

            void slotErrorFoundEvent(const std::string&, const std::string&);

            void slotEndErrorEvent();

            void slotReceiveNameEvent(const std::string&);

            void slotNewPluginAvailableEvent();

            void slotInbuildDevicesAvailableEvent();

            void slotStartDeviceEvent(const karabo::util::Config&);

            void slotTimeoutEvent(const std::string&);

            /**************************************************************/
            /*                        States                              */

            /**************************************************************/

            virtual void errorStateOnEntry() {
            }

            virtual void waitingForNameStateOnEntry() {
            }

            virtual void idleStateOnEntry() {
            }

            virtual void servingStateOnEntry() {
            }

            /**************************************************************/
            /*                    Transition Actions                      */

            /**************************************************************/

            virtual void errorFoundAction(const std::string&, const std::string&) {
            }

            virtual void endErrorAction() {
            }

            virtual void setNameAction(const std::string&) {
            }

            virtual void notifyNewDeviceAction() {
            }

            virtual void startDeviceAction(const karabo::util::Config&) {
            }

        private:

            void declareEventSlots();

            struct Impl;
            Impl* m_impl;

        };

    } // namespace core
} // namespace karabo

#endif 
