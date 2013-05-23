/*
 * $Id$
 *
 * Design, concepts and ideas: <serguei.essenov@xfel.eu>
 * This adapted implementation: <burkhard.heisen@xfel.eu>
 *
 * Created on March 23, 2011, 1:28 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "DeviceServerMachine.hh"
#include "DeviceServerLogic.hh"

namespace exfel {

    namespace core {

        using namespace std;
        using namespace exfel::util;
        using namespace deviceServer;

        struct DeviceServerLogic::Impl {


            Impl() : m_fsm(0) {
            }


            void start(DeviceServerLogic* p) {
                m_fsm = new TopMachine(p);
                m_fsm->start();
            }

            FSM_EVENT_IMPL2(ErrorFoundEvent, string, string)
            FSM_EVENT_IMPL0(EndErrorEvent)
            FSM_EVENT_IMPL1(ReceiveNameEvent, string)
            FSM_EVENT_IMPL1(TimeoutEvent, string)
            FSM_EVENT_IMPL0(InbuildDevicesAvailableEvent)
            FSM_EVENT_IMPL0(NewPluginAvailableEvent)
            FSM_EVENT_IMPL1(StartDeviceEvent, Config)

        private:
            TopMachine* m_fsm;
        };

        typedef exfel::core::DeviceServerLogic Self;


        FSM_EVENT_SLOT2(Self, ErrorFoundEvent, std::string, std::string)
        FSM_EVENT_SLOT0(Self, EndErrorEvent)
        FSM_EVENT_SLOT1(Self, ReceiveNameEvent, std::string)
        FSM_EVENT_SLOT1(Self, TimeoutEvent, std::string)
        FSM_EVENT_SLOT0(Self, NewPluginAvailableEvent)
        FSM_EVENT_SLOT0(Self, InbuildDevicesAvailableEvent)
        FSM_EVENT_SLOT1(Self, StartDeviceEvent, Config)

        void DeviceServerLogic::startStateMachine() {
            declareEventSlots();
            m_impl->start(this);
        }


        void DeviceServerLogic::declareEventSlots() {
            SLOT2(Self, slotErrorFoundEvent, std::string, std::string)
            SLOT0(Self, slotEndErrorEvent)
            SLOT1(Self, slotReceiveNameEvent, std::string)
            SLOT1(Self, slotStartDeviceEvent, Config)
        }



    } // namespace core
} // namespace exfel

