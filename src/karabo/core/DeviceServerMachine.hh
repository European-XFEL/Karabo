/*
 * $Id$
 *
 * Design, concepts and ideas: <serguei.essenov@xfel.eu>
 * This adapted implementation: <burkhard.heisen@xfel.eu>
 *
 * Created on March 22, 2011, 8:20 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_CORE_DEVICESERVERMACHINE_HH
#define	EXFEL_CORE_DEVICESERVERMACHINE_HH

#include "FsmMacros.hh"
#include "DeviceServerLogic.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

  namespace core {
    
    namespace deviceServer {

      
      /**************************************************************/
      /*                        Events                              */
      /**************************************************************/

      FSM_EVENT2(ErrorFoundEvent, std::string, std::string)

      FSM_EVENT0(EndErrorEvent)

      FSM_EVENT1(ReceiveNameEvent, std::string)

      FSM_EVENT1(TimeoutEvent, std::string)

      FSM_EVENT0(NewPluginAvailableEvent)

      FSM_EVENT0(InbuildDevicesAvailableEvent)

      FSM_EVENT1(StartDeviceEvent, exfel::util::Config)


      /**************************************************************/
      /*                        States                              */
      /**************************************************************/

      FSM_STATE_E(ErrorState, errorStateOnEntry)

      FSM_STATE_E(WaitingForNameState, waitingForNameStateOnEntry)

      FSM_STATE_E(IdleState, idleStateOnEntry)

      FSM_STATE_E(ServingState, servingStateOnEntry)

      /**************************************************************/
      /*                    Transition Actions                      */
      /**************************************************************/

      FSM_ACTION2(ErrorFoundAction, errorFoundAction)

      FSM_ACTION0(EndErrorAction, endErrorAction)

      FSM_ACTION1(SetNameAction, setNameAction)

      FSM_ACTION0(NotifyNewDeviceAction, notifyNewDeviceAction)

      FSM_ACTION1(StartDeviceAction, startDeviceAction)

      /**************************************************************/
      /*                      AllOk Machine                         */
      /**************************************************************/

      struct AllOkStateTransitionTable : mpl::vector<
        Row< WaitingForNameState, ReceiveNameEvent, IdleState, SetNameAction, none >,
        Row< WaitingForNameState, TimeoutEvent, IdleState, SetNameAction, none >,
        Row< IdleState, NewPluginAvailableEvent, none, NotifyNewDeviceAction, none >,
        Row< IdleState, InbuildDevicesAvailableEvent, none, NotifyNewDeviceAction, none >,
        Row< IdleState, StartDeviceEvent, ServingState,StartDeviceAction, none >,
        Row< ServingState, StartDeviceEvent, none, StartDeviceAction, none>
      > {
      };
      
      FSM_STATE_MACHINE(AllOkState, WaitingForNameState, DeviceServerLogic)


      /**************************************************************/
      /*                      Top Machine                           */
      /**************************************************************/

      struct TopMachineTransitionTable : mpl::vector<
                Row< AllOkState, ErrorFoundEvent, ErrorState, ErrorFoundAction, none >,
                Row< ErrorState, EndErrorEvent, AllOkState, EndErrorAction, none >
      > {
      };

      FSM_TOP_MACHINE(TopMachine, AllOkState, DeviceServerLogic)


    }
  } // namespace core
} // namespace exfel

#endif // EXFEL_CORE_DEVICESERVERMACHINE_HH
