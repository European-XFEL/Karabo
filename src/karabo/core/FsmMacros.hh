/*
 *
 * Author: esenov
 * Modified by: <burkhard.heisen@xfel.eu>
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

// clang-format off

#ifndef FSMMACROSDEVEL_HH
#define	FSMMACROSDEVEL_HH

#include <boost/shared_ptr.hpp>
// back-end
#include <boost/msm/back/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>
// functors
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
// for And_ operator
#include <boost/msm/front/euml/operator.hpp>
// for func_state and func_state_machine
#include <boost/msm/front/euml/state_grammar.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>

#include <karabo/log/Logger.hh>
#include "Worker.hh"
#include "FsmBaseState.hh"
#include <karabo/util/StateSignifier.hh>

// Allow boost msm names appear globally in karabo namespace
namespace karabo {
    using boost::msm::front::none;
    using boost::msm::front::Row;
}


/**************************************************************/
/*                    Special Macros                          */
/**************************************************************/

// Declares a shared pointer of a state machine
#define KARABO_FSM_DECLARE_MACHINE(machineName, instanceName) boost::shared_ptr<machineName> instanceName; boost::recursive_mutex instanceName ## _mutex;

// Instantiates a state machine as a shared pointer
#define KARABO_FSM_CREATE_MACHINE(machineName, instanceName) instanceName = boost::shared_ptr<machineName > (new machineName());

#define KARABO_FSM_TABLE_BEGIN(tableName) struct tableName : boost::mpl::vector<

#define KARABO_FSM_TABLE_END > {};

#define KARABO_FSM_SET_CONTEXT_TOP(ctx, top) top->setContext(ctx);

#define KARABO_FSM_SET_CONTEXT_SUB(ctx, parent, SubFsm) SubFsm* SubFsm##_ptr = parent->get_state<SubFsm* >(); SubFsm##_ptr->setContext(ctx);

#define KARABO_FSM_SET_CONTEXT_SUB1(context, fsmInstance, SubFsm1) fsmInstance->get_state<SubFsm1* >()->setContext(context);

#define KARABO_FSM_SET_CONTEXT_SUB2(context, fsmInstance, SubFsm1, SubFsm2) fsmInstance->get_state<SubFsm1* >()->get_state<SubFsm2* >()->setContext(context);

#define KARABO_FSM_SET_CONTEXT_SUB3(context, fsmInstance, SubFsm1, SubFsm2, SubFsm3) fsmInstance->get_state<SubFsm1* >()->get_state<SubFsm2* >()->get_state<SubFsm3* >()->setContext(context);

// Getting State pointer to the state in deeply nested state machines...
// It is possible to use in code KARABO_FSM_GET(3,A,B,C)->setContext(this);
#define KARABO_FSM_GET_DECLARE(machineName, instanceName) boost::shared_ptr<machineName> getFsm() { return instanceName; } \
    virtual void stopFsm() { boost::shared_ptr<StateVisitor> v(new StateVisitor); this->getFsm()->visit_current_states(v, true); }

#define KARABO_FSM_GET0() getFsm()
#define KARABO_FSM_GET1(A)            KARABO_FSM_GET0()->get_state<A*>()
#define KARABO_FSM_GET2(A,B)          KARABO_FSM_GET1(A)->get_state<B*>()
#define KARABO_FSM_GET3(A,B,C)        KARABO_FSM_GET2(A,B)->get_state<C*>()
#define KARABO_FSM_GET4(A,B,C,D)      KARABO_FSM_GET3(A,B,C)->get_state<D*>()
#define KARABO_FSM_GET5(A,B,C,D,E)    KARABO_FSM_GET4(A,B,C,D)->get_state<E*>()
#define KARABO_FSM_GET6(A,B,C,D,E,F)  KARABO_FSM_GET5(A,B,C,D,E)->get_state<F*>()

#define KARABO_FSM_GETN(n,...) KARABO_FSM_GET ## n(__VA_ARGS__)
#define KARABO_FSM_VSIZE(...) BOOST_PP_SUB(KARABO_FSM_VSIZE_I(e0,##__VA_ARGS__,10,9,8,7,6,5,4,3,2,1),1)
#define KARABO_FSM_VSIZE_I(e1,e2,e3,e4,e5,e6,e7,e8,e9,e10,N,...) N
#define KARABO_FSM_GET(...) BOOST_PP_CAT(KARABO_FSM_GET,KARABO_FSM_VSIZE(__VA_ARGS__))(__VA_ARGS__)

// Produces a static function that calls errorFunction of the state machine's context
#define KARABO_FSM_ON_EXCEPTION(errorFunction) \
template <class Fsm> \
static void _onError(const Fsm& fsm, const std::string& userFriendlyMsg = "Unknown error happened", const std::string& detailedMsg = "Unknown exception was triggered") { \
    fsm.getContext()->errorFunction(userFriendlyMsg, detailedMsg); \
}

// Produces a static function that calls stateChangeFunction of the state machine's context
#define KARABO_FSM_ON_CURRENT_STATE_CHANGE(stateChangeFunction) \
template <class Fsm> \
static void _updateCurrentState(Fsm& fsm, bool isGoingToChange = false) { \
    if (isGoingToChange) fsm.getContext()->stateChangeFunction(karabo::util::State::CHANGING); \
    else { \
        boost::shared_ptr<StateVisitor> v(new StateVisitor); \
        fsm.visit_current_states(v, false); \
        fsm.getContext()->stateChangeFunction(v->getState()->getState()); \
    } \
}

#define KARABO_FSM_START_MACHINE(machineInstance) machineInstance->start();_updateCurrentState(*machineInstance);

/**************************************************************/
/*                        Events                              */
/**************************************************************/

// 'm' - state machine pointer, 'name' - event name, 'f' - slot function, t1,t2,... - argument types
#define KARABO_FSM_EVENT0(m,name,f) \
    struct name {};\
    void f() { boost::recursive_mutex::scoped_lock lock(m ## _mutex); _updateCurrentState(*m, true); m->process_event(name()); _updateCurrentState(*m); }

#define KARABO_FSM_EVENT1(m,name,f,t1) \
    struct name { \
        name(const t1& b1) : a1(b1) {} \
        t1 a1; };\
    void f(const t1& c1) { boost::recursive_mutex::scoped_lock lock(m ## _mutex); _updateCurrentState(*m, true); m->process_event(name(c1));  _updateCurrentState(*m); }

#define KARABO_FSM_EVENT2(m,name,f,t1,t2) \
    struct name { \
        name(const t1& b1,const t2& b2) : a1(b1),a2(b2) {}\
        t1 a1; \
        t2 a2; }; \
    void f(const t1& c1, const t2& c2) { boost::recursive_mutex::scoped_lock lock(m ## _mutex); _updateCurrentState(*m, true); m->process_event(name(c1,c2)); _updateCurrentState(*m);}

#define KARABO_FSM_EVENT3(m,name,f,t1,t2,t3) \
    struct name { \
        name(const t1& b1,const t2& b2,const t3& b3) : a1(b1),a2(b2),a3(b3) {} \
        t1 a1; \
        t2 a2; \
        t3 a3; }; \
    void f(const t1& c1, const t2& c2, const t3& c3) { boost::recursive_mutex::scoped_lock lock(m ## _mutex); _updateCurrentState(*m, true); m->process_event(name(c1,c2,c3));_updateCurrentState(*m);}

#define KARABO_FSM_EVENT4(m,name,f,t1,t2,t3,t4) \
    struct name { \
        name(const t1& b1,const t2& b2,const t3& b3,const t4& b4) : a1(b1),a2(b2),a3(b3),a4(b4) {} \
        t1 a1; \
        t2 a2; \
        t3 a3; \
        t4 a4; };\
    void f(const t1& c1, const t2& c2, const t3& c3, const t4& c4) { boost::recursive_mutex::scoped_lock lock(m ## _mutex); _updateCurrentState(*m, true); m->process_event(name(c1,c2,c3,c4)); _updateCurrentState(*m);}


/**************************************************************/
/*                    Transition Actions                      */
/**************************************************************/

#define _KARABO_FSM_NO_TRANSITION_ACTION_IMPL(action) \
struct NoTransitionAction { \
    template <class Fsm, class Event> \
    void operator()(Event const& e, Fsm& f, int state) { \
        std::string type_id(typeid (e).name()); \
        /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
        KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #action; \
        f.getContext()->action(type_id,state); \
    } \
};

#define KARABO_FSM_ON_NO_STATE_TRANSITION(action) \
_KARABO_FSM_NO_TRANSITION_ACTION_IMPL(action);

#define KARABO_FSM_NO_TRANSITION_V_ACTION(action) \
_KARABO_FSM_NO_TRANSITION_ACTION_IMPL(action);\
virtual void action(const std::string& typeId, int state);
#define KARABO_FSM_NO_TRANSITION_VE_ACTION(action) \
_KARABO_FSM_NO_TRANSITION_ACTION_IMPL(action);\
virtual void action(const std::string& typeId, int state) {}
#define KARABO_FSM_NO_TRANSITION_PV_ACTION(action) \
_KARABO_FSM_NO_TRANSITION_ACTION_IMPL(action) \
virtual void action(const std::string& typeId, int state) = 0;

#define _KARABO_FSM_ACTION_IMPL0(name, func) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    void operator()(Evt const&, Fsm& f, SourceState&, TargetState&) { \
        try { \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name; \
            f.getContext()->func(); \
        } catch(karabo::util::Exception const& e) { \
            /* Order of calls to userFriendlyMsg(false) and detailedMsg() matters since the latter clears the stack,*/ \
            /* but C++ does not guarantee order of argument evaluation. Therefore cache user friendly message.*/ \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define KARABO_FSM_ACTION0(name, func) \
_KARABO_FSM_ACTION_IMPL0(name, func) \
void func();
#define KARABO_FSM_V_ACTION0(name, func) \
_KARABO_FSM_ACTION_IMPL0(name, func) \
virtual void func();
#define KARABO_FSM_VE_ACTION0(name, func) \
_KARABO_FSM_ACTION_IMPL0(name, func) \
virtual void func() {}
#define KARABO_FSM_PV_ACTION0(name, func) \
_KARABO_FSM_ACTION_IMPL0(name, func) \
virtual void func() = 0;

#define _KARABO_FSM_ACTION_IMPL1(name, func, t1) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    void operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name; \
            f.getContext()->func(e.a1); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define KARABO_FSM_V_ACTION1(name, func, t1) \
_KARABO_FSM_ACTION_IMPL1(name, func, t1) \
virtual void func(const t1&);

#define KARABO_FSM_PV_ACTION1(name, func, t1) \
_KARABO_FSM_ACTION_IMPL1(name, func, t1) \
virtual void func(const t1&)=0;
#define KARABO_FSM_VE_ACTION1(name, func, t1) \
_KARABO_FSM_ACTION_IMPL1(name, func, t1) \
virtual void func(const t1&) {}
#define _KARABO_FSM_ACTION_IMPL2(name, func, t1, t2) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    void operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name; \
            f.getContext()->func(static_cast<t1>(e.a1), static_cast<t2>(e.a2)); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define KARABO_FSM_V_ACTION2(name, func, t1, t2) \
_KARABO_FSM_ACTION_IMPL2(name, func, t1, t2) \
virtual void func(const t1&, const t2&);
#define KARABO_FSM_VE_ACTION2(name, func, t1, t2) \
_KARABO_FSM_ACTION_IMPL2(name, func, t1, t2) \
virtual void func(const t1&, const t2&) {}
#define KARABO_FSM_PV_ACTION2(name, func, t1, t2) \
_KARABO_FSM_ACTION_IMPL2(name, func, t1, t2) \
virtual void func(const t1&, const t2&) = 0;

#define _KARABO_FSM_ACTION_IMPL3(name, func, t1, t2, t3) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    void operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name; \
            f.getContext()->func(static_cast<t1>(e.a1), static_cast<t2>(e.a2), static_cast<t3>(e.a3)); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define KARABO_FSM_V_ACTION3(name, func, t1, t2, t3) \
_KARABO_FSM_ACTION_IMPL3(name, func, t1, t2, t3) \
virtual void func(const t1&,const t2&,const t3&);
#define KARABO_FSM_VE_ACTION3(name, func, t1, t2, t3) \
_KARABO_FSM_ACTION_IMPL3(name, func, t1, t2, t3) \
virtual void func(const t1&,const t2&,const t3&) {}
#define KARABO_FSM_PV_ACTION3(name, func, t1, t2, t3) \
_KARABO_FSM_ACTION_IMPL3(name, func, t1, t2, t3) \
virtual void func(const t1&,const t2&,const t3&) = 0;

#define _KARABO_FSM_ACTION_IMPL4(name, func, t1, t2, t3, t4) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    void operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name; \
            f.getContext()->func(static_cast<t1>(e.a1), static_cast<t2>(e.a2), static_cast<t3>(e.a3), static_cast<t4>(e.a4)); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define KARABO_FSM_V_ACTION4(name, func, t1, t2, t3, t4) \
_KARABO_FSM_ACTION_IMPL4(name, func, t1, t2, t3, t4) \
virtual void func(const t1&,const t2&,const t3&,const t4&);
#define KARABO_FSM_VE_ACTION4(name, func, t1, t2, t3, t4) \
_KARABO_FSM_ACTION_IMPL4(name, func, t1, t2, t3, t4) \
virtual void func(const t1&,const t2&,const t3&,const t4&) {}
#define KARABO_FSM_PV_ACTION4(name, func, t1, t2, t3, t4) \
_KARABO_FSM_ACTION_IMPL4(name, func, t1, t2, t3, t4) \
virtual void func(const t1&,const t2&,const t3&,const t4&) = 0;

/**************************************************************/
/*                        Guards                              */
/**************************************************************/

#define _KARABO_FSM_GUARD_IMPL0(name, func) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    bool operator()(Evt const&, Fsm& f, SourceState&, TargetState&) { \
        try { \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name; \
            return f.getContext()->func(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
        return false; \
    } \
};
#define KARABO_FSM_V_GUARD0(name, func) \
_KARABO_FSM_GUARD_IMPL0(name, func) \
virtual bool func();
#define KARABO_FSM_VE_GUARD0(name, func) \
_KARABO_FSM_GUARD_IMPL0(name, func) \
virtual bool func() {return true;}
#define KARABO_FSM_PV_GUARD0(name, func) \
_KARABO_FSM_GUARD_IMPL0(name, func) \
virtual bool func() = 0;

#define _KARABO_FSM_GUARD_IMPL1(name, func, t1) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    bool operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name; \
            return f.getContext()->func(static_cast<t1>(e.a1)); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
        return false; \
    } \
};
#define KARABO_FSM_V_GUARD1(name, func, t1) \
_KARABO_FSM_GUARD_IMPL1(name, func, t1) \
virtual bool func(const t1&);
#define KARABO_FSM_VE_GUARD1(name, func, t1) \
_KARABO_FSM_GUARD_IMPL1(name, func, t1) \
virtual bool func(const t1&) {return true;}
#define KARABO_FSM_PV_GUARD1(name, func, t1) \
_KARABO_FSM_GUARD_IMPL1(name, func, t1) \
virtual bool func(const t1&) = 0;

#define _KARABO_FSM_GUARD_IMPL2(name, func, t1, t2) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    bool operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name; \
            return f.getContext()->func(static_cast<t1>(e.a1), static_cast<t2>(e.a2)); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
        return false; \
    } \
};
#define KARABO_FSM_V_GUARD2(name, func, t1, t2) \
_KARABO_FSM_GUARD_IMPL2(name, func, t1, t2) \
virtual bool func(const t1&, const t2&);
#define KARABO_FSM_VE_GUARD2(name, func, t1, t2) \
_KARABO_FSM_GUARD_IMPL2(name, func, t1, t2) \
virtual bool func(const t1&, const t2&) {return true;}
#define KARABO_FSM_PV_GUARD2(name, func, t1, t2) \
_KARABO_FSM_GUARD_IMPL2(name, func, t1, t2) \
virtual bool func(const t1&, const t2&) = 0;

#define _KARABO_FSM_GUARD_IMPL3(name, func, t1, t2, t3) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    bool operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name; \
            return f.getContext()->func(static_cast<t1>(e.a1), e.a2, e.a3); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
        return false; \
    } \
};
#define KARABO_FSM_V_GUARD3(name, func, t1, t2, t3) \
_KARABO_FSM_GUARD_IMPL3(name, func, t1, t2, t3) \
virtual bool func(const t1&, const t2&, const t3&);
#define KARABO_FSM_VE_GUARD3(name, func, t1, t2, t3) \
_KARABO_FSM_GUARD_IMPL3(name, func, t1, t2, t3) \
virtual bool func(const t1&, const t2&, const t3&) {return true;}
#define KARABO_FSM_PV_GUARD3(name, func, t1, t2, t3) \
_KARABO_FSM_GUARD_IMPL3(name, func, t1, t2, t3) \
virtual bool func(const t1&, const t2&, const t3&) = 0;

#define _KARABO_FSM_GUARD_IMPL4(name, func, t1, t2, t3, t4) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    bool operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name; \
            return f.getContext()->func(static_cast<t1>(e.a1), static_cast<t2>(e.a2), static_cast<t3>(e.a3), static_cast<t4>(e.a4)); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
        return false; \
    } \
};
#define KARABO_FSM_V_GUARD4(name, func, a1, a2, a3, a4) \
_KARABO_FSM_GUARD_IMPL4(name, func, t1, t2, t3, t4) \
virtual bool func(const t1&, const t2&, const t3&);
#define KARABO_FSM_VE_GUARD4(name, func, a1, a2, a3, a4) \
_KARABO_FSM_GUARD_IMPL4(name, func, t1, t2, t3, t4) \
virtual bool func(const t1&, const t2&, const t3&) {return true;}
#define KARABO_FSM_PV_GUARD4(name, func, a1, a2, a3, a4) \
_KARABO_FSM_GUARD_IMPL4(name, func, t1, t2, t3, t4) \
virtual bool func(const t1&, const t2&, const t3&);


/**************************************************************/
/*                 State Periodic Action                      */
/**************************************************************/

// WARNING: The following macro may be used only in classes where KARABO_CLASSINFO is used.
// It relies on the fact that 'Self' is properly defined (typedef to the class type)

#define _KARABO_FSM_PERIODIC_ACTION_IMPL0(name, timeout, repetition, func) \
struct name { \
    name() : m_timeout(timeout), m_repetition(repetition), m_context(0) {} \
    void setContext(Self* const ctx) {if (!ctx) KARABO_LOG_FRAMEWORK_DEBUG << #name << ": WARNING set context  as NULL."; m_context = ctx; } \
    void operator()() { \
            m_context->func(); \
    } \
    int getTimeout() const { return m_timeout; } \
    int getRepetition() const { return m_repetition; } \
private: \
    int m_timeout; \
    int m_repetition; \
    Self* m_context; \
};
#define KARABO_FSM_PERIODIC_ACTION(name, timeout, repetition, func) \
_KARABO_FSM_PERIODIC_ACTION_IMPL0(name, timeout, repetition, func) \
void func();
#define KARABO_FSM_V_PERIODIC_ACTION(name, timeout, repetition, func) \
_KARABO_FSM_PERIODIC_ACTION_IMPL0(name, timeout, repetition, func) \
virtual void func();
#define KARABO_FSM_VE_PERIODIC_ACTION(name, timeout, repetition, func) \
_KARABO_FSM_PERIODIC_ACTION_IMPL0(name, timeout, repetition, func) \
virtual void func() {}
#define KARABO_FSM_PV_PERIODIC_ACTION(name, timeout, repetition, func) \
_KARABO_FSM_PERIODIC_ACTION_IMPL0(name, timeout, repetition, func) \
virtual void func() = 0;


/**************************************************************/
/*                        States                              */
/**************************************************************/

#define KARABO_FSM_STATE(name) struct name : public boost::msm::front::state<karabo::core::FsmBaseState> { \
name() {this->setState(karabo::util::State::name);} \
template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
    try { \
        this->setFsmName(f.getFsmName()); \
        this->setContained(f.is_contained()); \
        /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
        KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
    } catch(karabo::util::Exception const& e) { \
        const std::string friendlyMsg(e.userFriendlyMsg(false)); \
        _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
    } catch(...) { \
        _onError<Fsm>(f); \
    }\
} \
};

#define KARABO_FSM_STATE_A(name, TargetAction) \
struct name : public boost::msm::front::state<karabo::core::FsmBaseState> { \
    TargetAction _ta; \
    boost::shared_ptr<Worker> _worker; \
    name() : _ta(), _worker(new Worker) {this->setState(karabo::util::State::name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            this->setContained(f.is_contained()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            _ta.setContext(f.getContext()); \
            _worker->set(_ta, this->getTimeout(), this->getRepetition()).start(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": exit"; \
            _worker->stop().join(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    Worker* getWorker() const  {return _worker.get();} \
};

#define _KARABO_FSM_STATE_IMPL_E(name, entryFunc) \
struct name : public boost::msm::front::state<karabo::core::FsmBaseState> { \
    name() {this->setState(karabo::util::State::name);} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
             this->setContained(f.is_contained()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define KARABO_FSM_STATE_V_E(name, entryFunc) \
_KARABO_FSM_STATE_IMPL_E(name, entryFunc) \
virtual void entryFunc();
#define KARABO_FSM_STATE_VE_E(name, entryFunc) \
_KARABO_FSM_STATE_IMPL_E(name, entryFunc) \
virtual void entryFunc() {}
#define KARABO_FSM_STATE_PV_E(name, entryFunc) \
_KARABO_FSM_STATE_IMPL_E(name, entryFunc) \
virtual void entryFunc() = 0;

#define _KARABO_FSM_STATE_IMPL_AE(name, TargetAction, entryFunc) \
struct name : public boost::msm::front::state<karabo::core::FsmBaseState> { \
    TargetAction _ta; \
    boost::shared_ptr<Worker> _worker; \
    name() : _ta(), _worker(new Worker) {this->setState(karabo::util::State::name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            this->setContained(f.is_contained()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(); \
            _ta.setContext(f.getContext()); \
            _worker->set(_ta, this->getTimeout(), this->getRepetition()).start(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": exit"; \
            _worker->stop().join(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    Worker* getWorker() const  {return _worker.get();} \
};
#define KARABO_FSM_STATE_V_AE(name, TargetAction, entryFunc) \
_KARABO_FSM_STATE_IMPL_AE(name, TargetAction, entryFunc) \
virtual void entryFunc();
#define KARABO_FSM_STATE_VE_AE(name, TargetAction, entryFunc) \
_KARABO_FSM_STATE_IMPL_AE(name, TargetAction, entryFunc) \
virtual void entryFunc() {}
#define KARABO_FSM_STATE_PV_AE(name, TargetAction, entryFunc) \
_KARABO_FSM_STATE_IMPL_AE(name, TargetAction, entryFunc) \
virtual void entryFunc() = 0;

#define _KARABO_FSM_STATE_IMPL_E1(name, entryFunc, event, t1) \
struct name : public boost::msm::front::state<karabo::core::FsmBaseState> { \
    name() {this->setState(karabo::util::State::name);} \
    template <class Event, class Fsm> void on_entry(const Event& e, Fsm& f) {} \
    template <class Fsm> void on_entry(event const& e, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(static_cast<t1>(e.a1)); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};

#define KARABO_FSM_STATE_V_E1(name, entryFunc, event, t1) \
_KARABO_FSM_STATE_IMPL_E1(name, entryFunc, event, t1) \
virtual void entryFunc(const t1&);
#define KARABO_FSM_STATE_VE_E1(name, entryFunc, event, t1) \
_KARABO_FSM_STATE_IMPL_E1(name, entryFunc, event, t1) \
virtual void entryFunc(const t1&) {}
#define KARABO_FSM_STATE_PV_E1(name, entryFunc, event, t1) \
_KARABO_FSM_STATE_IMPL_E1(name, entryFunc, event, t1) \
virtual void entryFunc(const t1&) = 0;

#define _KARABO_FSM_STATE_IMPL_E2(name, entryFunc, t1, t2) \
struct name : public boost::msm::front::state<karabo::core::FsmBaseState> { \
    name() {this->setState(karabo::util::State::name);} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(static_cast<t1>(e.a1), static_cast<t2>(e.a2)); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define KARABO_FSM_STATE_V_E2(name, entryFunc, t1, t2) \
_KARABO_FSM_STATE_IMPL_E2(name, entryFunc, t1, t2) \
virtual void entryFunc(const t1&, const t2&);
#define KARABO_FSM_STATE_VE_E2(name, entryFunc, t1, t2) \
_KARABO_FSM_STATE_IMPL_E2(name, entryFunc, t1, t2) \
virtual void entryFunc(const t1&, const t2&) {}
#define KARABO_FSM_STATE_PV_E2(name, entryFunc, t1, t2) \
_KARABO_FSM_STATE_IMPL_E2(name, entryFunc, t1, t2) \
virtual void entryFunc(const t1&, const t2&) = 0;

#define _KARABO_FSM_STATE_IMPL_E3(name, entryFunc, t1, t2, t3) \
struct name : public boost::msm::front::state<karabo::core::FsmBaseState> { \
    name() {this->setState(karabo::util::State::name);} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(static_cast<t1>(e.a1), static_cast<t2>(e.a2), static_cast<t3>(e.a3)); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    private: std::string m_state; \
};
#define KARABO_FSM_STATE_V_E3(name, entryFunc, t1, t2, t3) \
_KARABO_FSM_STATE_IMPL_E3(name, entryFunc, t1, t2, t3) \
virtual void entryFunc(const t1&, const t2&, const t3&);
#define KARABO_FSM_STATE_VE_E3(name, entryFunc, t1, t2, t3) \
_KARABO_FSM_STATE_IMPL_E3(name, entryFunc, t1, t2, t3) \
virtual void entryFunc(const t1&, const t2&, const t3&) {}
#define KARABO_FSM_STATE_PV_E3(name, entryFunc, t1, t2, t3) \
_KARABO_FSM_STATE_IMPL_E3(name, entryFunc, t1, t2, t3) \
virtual void entryFunc(const t1&, const t2&, const t3&) = 0;


#define _KARABO_FSM_STATE_IMPL_E4(name, entryFunc, t1, t2, t3, t4) \
struct name : public boost::msm::front::state<karabo::core::FsmBaseState> { \
    name() {this->setState(karabo::util::State::name));} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(static_cast<t1>(e.a1), static_cast<t2>(e.a2), static_cast<t3>(e.a3), static_cast<t4>(e.a4)); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define KARABO_FSM_STATE_V_E4(name, entryFunc, t1, t2, t3, t4) \
_KARABO_FSM_STATE_IMPL_E4(name, entryFunc, t1, t2, t3, t4) \
virtual void entryFunc(const t1&, const t2&, const t3&, const t4&);
#define KARABO_FSM_STATE_VE_E4(name, entryFunc, t1, t2, t3, t4) \
_KARABO_FSM_STATE_IMPL_E4(name, entryFunc, t1, t2, t3, t4) \
virtual void entryFunc(const t1&, const t2&, const t3&, const t4&) {}
#define KARABO_FSM_STATE_PV_E4(name, entryFunc, t1, t2, t3, t4) \
_KARABO_FSM_STATE_IMPL_E4(name, entryFunc, t1, t2, t3, t4) \
virtual void entryFunc(const t1&, const t2&, const t3& c3, const t4&) = 0;



#define _KARABO_FSM_STATE_IMPL_EE(name, entryFunc, exitFunc) \
struct name : public boost::msm::front::state<karabo::core::FsmBaseState> { \
    name() {this->setState(karabo::util::State::name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": exit"; \
            f.getContext()->exitFunc(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define KARABO_FSM_STATE_V_EE(name, entryFunc, exitFunc) \
_KARABO_FSM_STATE_IMPL_EE(name, entryFunc, exitFunc) \
virtual void entryFunc(); \
virtual void exitFunc();
#define KARABO_FSM_STATE_VE_EE(name, entryFunc, exitFunc) \
_KARABO_FSM_STATE_IMPL_EE(name, entryFunc, exitFunc) \
virtual void entryFunc() {} \
virtual void exitFunc() {}
#define KARABO_FSM_STATE_PV_EE(name, entryFunc, exitFunc) \
_KARABO_FSM_STATE_IMPL_EE(name, entryFunc, exitFunc) \
virtual void entryFunc() = 0; \
virtual void exitFunc()  = 0;

#define _KARABO_FSM_STATE_IMPL_AEE(name, TargetAction, entryFunc, exitFunc) \
struct name : public boost::msm::front::state<karabo::core::FsmBaseState> { \
private: \
    TargetAction _ta; \
    boost::shared_ptr<Worker> _worker; \
public: \
    name() : _ta(), _worker(new Worker) {this->setState(karabo::util::State::name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(); \
            _ta.setContext(f.getContext()); \
            _worker->set(_ta, this->getTimeout(), this->getRepetition()).start(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            _worker->stop().join(); \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": exit"; \
            f.getContext()->exitFunc(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    Worker* getWorker() const  {return _worker.get();} \
};
#define KARABO_FSM_STATE_V_AEE(name, targetFunc, entryFunc, exitFunc) \
_KARABO_FSM_STATE_IMPL_AEE(name, targetFunc, entryFunc, exitFunc) \
virtual void entryFunc(); \
virtual void exitFunc();
#define KARABO_FSM_STATE_VE_AEE(name, targetFunc, entryFunc, exitFunc) \
_KARABO_FSM_STATE_IMPL_AEE(name, targetFunc, entryFunc, exitFunc) \
virtual void entryFunc() {} \
virtual void exitFunc() {}
#define KARABO_FSM_STATE_PV_AEE(name, targetFunc, entryFunc, exitFunc) \
_KARABO_FSM_STATE_IMPL_AEE(name, targetFunc, entryFunc, exitFunc) \
virtual void entryFunc() = 0; \
virtual void exitFunc()  = 0;

#define KARABO_FSM_TERMINATE_STATE(name) \
struct name : public boost::msm::front::terminate_state<karabo::core::FsmBaseState> { \
    name() {this->setState(karabo::util::State::name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};


#define _KARABO_FSM_TERMINATE_STATE_IMPL_E(name, entryFunc) \
struct name : public boost::msm::front::terminate_state<karabo::core::FsmBaseState> { \
    name() {this->setState(karabo::util::State::name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define KARABO_FSM_TERMINATE_STATE_V_E(name, entryFunc) \
_KARABO_FSM_TERMINATE_STATE_IMPL_E(name, entryFunc) \
virtual void entryFunc();
#define KARABO_FSM_TERMINATE_STATE_VE_E(name, entryFunc) \
_KARABO_FSM_TERMINATE_STATE_IMPL_E(name, entryFunc) \
virtual void entryFunc() {}
#define KARABO_FSM_TERMINATE_STATE_PV_E(name, entryFunc) \
_KARABO_FSM_TERMINATE_STATE_IMPL_E(name, entryFunc) \
virtual void entryFunc() = 0;


#define _KARABO_FSM_TERMINATE_STATE_IMPL_EE(name, entryFunc, exitFunc) \
struct name : public boost::msm::front::terminate_state<karabo::core::FsmBaseState> { \
    name() {this->setState(karabo::util::State::name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": exit"; \
            f.getContext()->exitFunc();  \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define KARABO_FSM_TERMINATE_STATE_V_EE(name, entryFunc, exitFunc) \
_KARABO_FSM_TERMINATE_STATE_IMPL_EE(name, entryFunc, exitFunc) \
virtual void entryFunc(); \
virtual void exitFunc();
#define KARABO_FSM_TERMINATE_STATE_VE_EE(name, entryFunc, exitFunc) \
_KARABO_FSM_TERMINATE_STATE_IMPL_EE(name, entryFunc, exitFunc) \
virtual void entryFunc() {} \
virtual void exitFunc() {}
#define KARABO_FSM_TERMINATE_STATE_PV_EE(name, entryFunc, exitFunc) \
_KARABO_FSM_TERMINATE_STATE_IMPL_EE(name, entryFunc, exitFunc) \
virtual void entryFunc() = 0; \
virtual void exitFunc()  = 0;


#define KARABO_FSM_INTERRUPT_STATE(name, event) \
struct name : public boost::msm::front::interrupt_state<event, karabo::core::FsmBaseState > { \
    name() {this->setState(karabo::util::State::name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};


#define KARABO_FSM_INTERRUPT_STATE_A(name, event, TargetAction) \
struct name : public boost::msm::front::interrupt_state<event, karabo::core::FsmBaseState > { \
    TargetAction _ta; \
    boost::shared_ptr<Worker> _worker; \
    name() : _ta(), _worker(new Worker) {this->setState(karabo::util::State::name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            _ta.setContext(f.getContext()); \
            _worker->set(_ta, this->getTimeout(), this->getRepetition()).start(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": exit"; \
            _worker->stop().join(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    Worker* getWorker() const  {return _worker.get();} \
};


#define _KARABO_FSM_INTERRUPT_STATE_IMPL_E(name, event, entryFunc) \
struct name : public boost::msm::front::interrupt_state<event, karabo::core::FsmBaseState > { \
    name() {this->setState(karabo::util::State::name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define KARABO_FSM_INTERRUPT_STATE_V_E(name, event, entryFunc) \
_KARABO_FSM_INTERRUPT_STATE_IMPL_E(name, event, entryFunc) \
virtual void entryFunc();
#define KARABO_FSM_INTERRUPT_STATE_VE_E(name, event, entryFunc) \
_KARABO_FSM_INTERRUPT_STATE_IMPL_E(name, event, entryFunc) \
virtual void entryFunc() {}
#define KARABO_FSM_INTERRUPT_STATE_PV_E(name, event, entryFunc) \
_KARABO_FSM_INTERRUPT_STATE_IMPL_E(name, event, entryFunc) \
virtual void entryFunc() = 0;


#define _KARABO_FSM_INTERRUPT_STATE_IMPL_AE(name, event, TargetAction, entryFunc) \
struct name : public boost::msm::front::interrupt_state<event, karabo::core::FsmBaseState > { \
    TargetAction _ta; \
    boost::shared_ptr<Worker> _worker; \
    name() : _ta(), _worker(new Worker) {this->setState(karabo::util::State::name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(); \
            _ta.setContext(f.getContext()); \
            _worker->set(_ta, this->getTimeout(), this->getRepetition()).start(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": exit"; \
            _worker->stop().join(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    Worker* getWorker() const  {return _worker.get();} \
};
#define KARABO_FSM_INTERRUPT_STATE_V_AE(name, event, TargetAction, entryFunc) \
_KARABO_FSM_INTERRUPT_STATE_IMPL_AE(name, event, TargetAction, entryFunc) \
virtual void entryFunc();
#define KARABO_FSM_INTERRUPT_STATE_VE_AE(name, event, TargetAction, entryFunc) \
_KARABO_FSM_INTERRUPT_STATE_IMPL_AE(name, event, TargetAction, entryFunc) \
virtual void entryFunc() {}
#define KARABO_FSM_INTERRUPT_STATE_PV_AE(name, event, TargetAction, entryFunc) \
_KARABO_FSM_INTERRUPT_STATE_IMPL_AE(name, event, TargetAction, entryFunc) \
virtual void entryFunc() = 0;


#define _KARABO_FSM_INTERRUPT_STATE_IMPL_EE(name, event, entryFunc, exitFunc) \
struct name : public boost::msm::front::interrupt_state<event, karabo::core::FsmBaseState > { \
    name() {this->setState(karabo::util::State::name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": exit"; \
            f.getContext()->exitFunc(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define KARABO_FSM_INTERRUPT_STATE_V_EE(name, event, entryFunc, exitFunc) \
_KARABO_FSM_INTERRUPT_STATE_IMPL_EE(name, event, entryFunc, exitFunc) \
virtual void entryFunc(); \
virtual void exitFunc();
#define KARABO_FSM_INTERRUPT_STATE_VE_EE(name, event, entryFunc, exitFunc) \
_KARABO_FSM_INTERRUPT_STATE_IMPL_EE(name, event, entryFunc, exitFunc) \
virtual void entryFunc() {} \
virtual void exitFunc() {}
#define KARABO_FSM_INTERRUPT_STATE_PV_EE(name, event, entryFunc, exitFunc) \
_KARABO_FSM_INTERRUPT_STATE_IMPL_EE(name, event, entryFunc, exitFunc) \
virtual void entryFunc() = 0; \
virtual void exitFunc()  = 0;


#define _KARABO_FSM_INTERRUPT_STATE_IMPL_AEE(name, event, TargetAction, entryFunc, exitFunc) \
struct name : public boost::msm::front::interrupt_state<event, karabo::core::FsmBaseState > { \
    TargetAction _ta; \
    boost::shared_ptr<Worker> _worker; \
    name() : _ta(), _worker(new Worker) {this->setState(karabo::util::State::name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(); \
            _ta.setContext(f.getContext()); \
            _worker->set(_ta, this->getTimeout(), this->getRepetition()).start(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            _worker->stop().join(); \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": exit"; \
            f.getContext()->exitFunc(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    Worker* getWorker() const  {return _worker.get();} \
};
#define KARABO_FSM_INTERRUPT_STATE_V_AEE(name, event, TargetAction, entryFunc, exitFunc) \
_KARABO_FSM_INTERRUPT_STATE_IMPL_AEE(name, event, TargetAction, entryFunc, exitFunc) \
virtual void entryFunc(); \
virtual void exitFunc();
#define KARABO_FSM_INTERRUPT_STATE_VE_AEE(name, event, TargetAction, entryFunc, exitFunc) \
_KARABO_FSM_INTERRUPT_STATE_IMPL_AEE(name, event, TargetAction, entryFunc, exitFunc) \
virtual void entryFunc() {} \
virtual void exitFunc() {}
#define KARABO_FSM_INTERRUPT_STATE_PV_AEE(name, event, TargetAction, entryFunc, exitFunc) \
_KARABO_FSM_INTERRUPT_STATE_IMPL_AEE(name, event, TargetAction, entryFunc, exitFunc) \
virtual void entryFunc() = 0; \
virtual void exitFunc()  = 0;


#define KARABO_FSM_EXIT_PSEUDO_STATE(name, event) struct name : public boost::msm::front::exit_pseudo_state<event, karabo::core::FsmBaseState > { \
name() {this->setState(karabo::util::State::name);} \
template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
    try { \
        this->setFsmName(f.getFsmName()); \
        /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
        KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
    } catch(karabo::util::Exception const& e) { \
        const std::string friendlyMsg(e.userFriendlyMsg(false)); \
        _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
    } catch(...) { \
        _onError<Fsm>(f); \
    }\
} \
};

#define _KARABO_FSM_EXIT_PSEUDO_STATE_IMPL_E(name, event, entryFunc) \
struct name : public boost::msm::front::exit_pseudo_state<event, karabo::core::FsmBaseState > { \
    name() {this->setState(karabo::util::State::name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define KARABO_FSM_EXIT_PSEUDO_STATE_V_E(name, event, entryFunc) \
_KARABO_FSM_EXIT_PSEUDO_STATE_IMPL_E(name, event, entryFunc) \
virtual void entryFunc();
#define KARABO_FSM_EXIT_PSEUDO_STATE_VE_E(name, event, entryFunc) \
_KARABO_FSM_EXIT_PSEUDO_STATE_IMPL_E(name, event, entryFunc) \
virtual void entryFunc() {}
#define KARABO_FSM_EXIT_PSEUDO_STATE_PV_E(name, event, entryFunc) \
_KARABO_FSM_EXIT_PSEUDO_STATE_IMPL_E(name, event, entryFunc) \
virtual void entryFunc() = 0;



#define _KARABO_FSM_EXIT_PSEUDO_STATE_IMPL_EE(name, event, entryFunc, exitFunc) \
struct name : public boost::msm::front::exit_pseudo_state<event, karabo::core::FsmBaseState > { \
    name() {this->setState(karabo::util::State::name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            this->setFsmName(f.getFsmName()); \
            /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
            KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": exit"; \
            f.getContext()->exitFunc(); \
        } catch(karabo::util::Exception const& e) { \
            const std::string friendlyMsg(e.userFriendlyMsg(false)); \
            _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define KARABO_FSM_EXIT_PSEUDO_STATE_V_EE(name, event, entryFunc, exitFunc) \
_KARABO_FSM_EXIT_PSEUDO_STATE_IMPL_EE(name, event, entryFunc, exitFunc) \
virtual void entryFunc(); \
virtual void exitFunc();
#define KARABO_FSM_EXIT_PSEUDO_STATE_VE_EE(name, event, entryFunc, exitFunc) \
_KARABO_FSM_EXIT_PSEUDO_STATE_IMPL_EE(name, event, entryFunc, exitFunc) \
virtual void entryFunc() {} \
virtual void exitFunc() {}
#define KARABO_FSM_EXIT_PSEUDO_STATE_PV_EE(name, event, entryFunc, exitFunc) \
_KARABO_FSM_EXIT_PSEUDO_STATE_IMPL_EE(name, event, entryFunc, exitFunc) \
virtual void entryFunc() = 0; \
virtual void exitFunc()  = 0;

/**************************************************************/
/*                         Machines                           */
/**************************************************************/

#define KARABO_FSM_REGION(x,y) boost::mpl::vector< x,y >

#define KARABO_FSM_STATE_MACHINE(name, stt, istate, context) \
        template<class CTX = context> \
        struct name ## _ : boost::msm::front::state_machine_def<name ## _<CTX>, karabo::core::FsmBaseState > { \
            typedef boost::msm::active_state_switch_after_transition_action active_state_switch_policy; \
            template <class Event, class Fsm> void on_entry(Event const& e, Fsm& f) { \
                try { \
                    /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
                    KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
                } catch(karabo::util::Exception const& e) { \
                    const std::string friendlyMsg(e.userFriendlyMsg(false)); \
                    _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
                } catch(...) { \
                    _onError<Fsm>(f); \
                }\
            } \
            template <class Fsm, class Event> void no_transition(Event const& e, Fsm& f, int state) { NoTransitionAction()(e,f,state); } \
            typedef stt transition_table; \
            typedef istate initial_state; \
            void setContext(CTX* const ctx) { m_context = ctx; } \
            CTX * const getContext() const { return m_context; } \
            name ## _() : m_context(0) {this->setStateMachineName(#name); this->setFsmName(#name);} \
        private: \
            CTX* m_context; \
        }; \
        typedef boost::msm::back::state_machine<name ## _<> > name;

#define KARABO_FSM_STATE_MACHINE_A(name, stt, istate, context, TargetAction) \
        template<class CTX = context> \
        struct name ## _ : boost::msm::front::state_machine_def<name ## _<CTX>, karabo::core::FsmBaseState > { \
            typedef boost::msm::active_state_switch_after_transition_action active_state_switch_policy; \
            template <class Event, class Fsm> void on_entry(Event const& e, Fsm& f) { \
                try { \
                    /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
                    KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
                    _ta.setContext(f.getContext()); \
                    _worker->set(_ta, this->getTimeout(), this->getRepetition()).start(); \
                } catch(karabo::util::Exception const& e) { \
                    const std::string friendlyMsg(e.userFriendlyMsg(false)); \
                    _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
                } catch(...) { \
                    _onError<Fsm>(f); \
                }\
            } \
            template <class Event, class Fsm> void on_exit (Event const& e, Fsm& f) { \
                try { \
                    _worker->stop().join(); \
                    /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
                    KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": exit"; \
                } catch(karabo::util::Exception const& e) { \
                    const std::string friendlyMsg(e.userFriendlyMsg(false)); \
                    _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
                } catch(...) { \
                _onError<Fsm>(f); \
                }\
            } \
            template <class Fsm, class Event> void no_transition(Event const& e, Fsm& f, int state) { NoTransitionAction()(e,f,state); } \
            typedef stt transition_table; \
            typedef istate initial_state; \
            void setContext(CTX* const ctx) { m_context = ctx; } \
            CTX * const getContext() const { return m_context; } \
            name ## _() : _ta(), _worker(new Worker), m_context(0) {this->setStateMachineName(#name); this->setFsmName(#name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
            Worker* getWorker() const  {return _worker.get();} \
        private: \
            TargetAction _ta; \
            boost::shared_ptr<Worker> _worker; \
            CTX* m_context; \
        }; \
        typedef boost::msm::back::state_machine<name ## _<> > name;

#define _KARABO_FSM_STATE_MACHINE_IMPL_E(name, stt, istate, context, entryFunc) \
        template<class CTX = context> \
        struct name ## _ : boost::msm::front::state_machine_def<name ## _<CTX>, karabo::core::FsmBaseState > { \
            typedef boost::msm::active_state_switch_after_transition_action active_state_switch_policy; \
            template <class Event, class Fsm> void on_entry(Event const& e, Fsm& f) { \
                try { \
                    /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
                    KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
                    f.getContext()->entryFunc(); \
                } catch(karabo::util::Exception const& e) { \
                    const std::string friendlyMsg(e.userFriendlyMsg(false)); \
                    _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
                } catch(...) { \
                    _onError<Fsm>(f); \
                }\
            } \
            template <class Fsm, class Event> void no_transition(Event const& e, Fsm& f, int state) { NoTransitionAction()(e,f,state); } \
            typedef stt transition_table; \
            typedef istate initial_state; \
            void setContext(CTX* const ctx) { m_context = ctx; } \
            CTX * const getContext() const { return m_context; } \
            name ## _() : m_context(0) {this->setStateMachineName(#name); this->setFsmName(#name);} \
        private: \
            CTX* m_context; \
        }; \
        typedef boost::msm::back::state_machine<name ## _<> > name;
#define KARABO_FSM_STATE_MACHINE_V_E(name, stt, istate, context, entryFunc) \
_KARABO_FSM_STATE_MACHINE_IMPL_E(name, stt, istate, context, entryFunc) \
virtual void entryFunc();
#define KARABO_FSM_STATE_MACHINE_VE_E(name, stt, istate, context, entryFunc) \
_KARABO_FSM_STATE_MACHINE_IMPL_E(name, stt, istate, context, entryFunc) \
virtual void entryFunc() {}
#define KARABO_FSM_STATE_MACHINE_PV_E(name, stt, istate, context, entryFunc) \
_KARABO_FSM_STATE_MACHINE_IMPL_E(name, stt, istate, context, entryFunc) \
virtual void entryFunc() = 0;


#define _KARABO_FSM_STATE_MACHINE_IMPL_AE(name, stt, istate, context, TargetAction, entryFunc) \
        template<class CTX = context> \
        struct name ## _ : boost::msm::front::state_machine_def<name ## _<CTX>, karabo::core::FsmBaseState > { \
            typedef boost::msm::active_state_switch_after_transition_action active_state_switch_policy; \
            template <class Event, class Fsm> void on_entry(Event const& e, Fsm& f) { \
                try { \
                    /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
                    KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
                    f.getContext()->entryFunc(); \
                    _ta.setContext(f.getContext()); \
                    _worker->set(_ta, this->getTimeout(), this->getRepetition()).start(); \
                } catch(karabo::util::Exception const& e) { \
                    const std::string friendlyMsg(e.userFriendlyMsg(false)); \
                    _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
                } catch(...) { \
                    _onError<Fsm>(f); \
                }\
            } \
            template <class Event, class Fsm> void on_exit (Event const& e, Fsm& f) { \
                try { \
                    _worker->stop().join(); \
                    /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
                    KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": exit"; \
                } catch(karabo::util::Exception const& e) { \
                    const std::string friendlyMsg(e.userFriendlyMsg(false)); \
                    _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
                } catch(...) { \
                    _onError<Fsm>(f); \
                }\
            } \
            template <class Fsm, class Event> void no_transition(Event const& e, Fsm& f, int state) { NoTransitionAction()(e,f,state); } \
            typedef stt transition_table; \
            typedef istate initial_state; \
            void setContext(CTX* const ctx) { m_context = ctx; } \
            CTX * const getContext() const { return m_context; } \
            name ## _() : _ta(), _worker(new Worker), m_context(0) {this->setStateMachineName(#name); this->setFsmName(#name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
            Worker* getWorker() const  {return _worker.get();} \
        private: \
            TargetAction _ta; \
            boost::shared_ptr<Worker> _worker; \
            CTX* m_context; \
        }; \
        typedef boost::msm::back::state_machine<name ## _<> > name;
#define KARABO_FSM_STATE_MACHINE_V_AE(name, stt, istate, context, TargetAction, entryFunc) \
_KARABO_FSM_STATE_MACHINE_IMPL_AE(name, stt, istate, context, TargetAction, entryFunc) \
virtual void entryFunc();
#define KARABO_FSM_STATE_MACHINE_VE_AE(name, stt, istate, context, TargetAction, entryFunc) \
_KARABO_FSM_STATE_MACHINE_IMPL_AE(name, stt, istate, context, TargetAction, entryFunc) \
virtual void entryFunc() {}
#define KARABO_FSM_STATE_MACHINE_PV_AE(name, stt, istate, context, TargetAction, entryFunc) \
_KARABO_FSM_STATE_MACHINE_IMPL_AE(name, stt, istate, context, TargetAction, entryFunc) \
virtual void entryFunc() = 0;


#define _KARABO_FSM_STATE_MACHINE_IMPL_EE(name, stt, istate, context, entryFunc, exitFunc) \
        template<class CTX = context> \
        struct name ## _ : boost::msm::front::state_machine_def<name ## _<CTX>, karabo::core::FsmBaseState > { \
            typedef boost::msm::active_state_switch_after_transition_action active_state_switch_policy; \
            template <class Event, class Fsm> void on_entry(Event const& e, Fsm& f) { \
                try { \
                    /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
                    KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
                    f.getContext()->entryFunc(); \
                } catch(karabo::util::Exception const& e) { \
                    const std::string friendlyMsg(e.userFriendlyMsg(false)); \
                    _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
                } catch(...) { \
                    _onError<Fsm>(f); \
                }\
            } \
            template <class Event, class Fsm> void on_exit (Event const& e, Fsm& f) { \
                try { \
                    /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
                    KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": exit"; \
                    f.getContext()->exitFunc(); \
                } catch(karabo::util::Exception const& e) { \
                    const std::string friendlyMsg(e.userFriendlyMsg(false)); \
                    _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
                } catch(...) { \
                    _onError<Fsm>(f); \
                }\
            } \
            template <class Fsm, class Event> void no_transition(Event const& e, Fsm& f, int state) { NoTransitionAction()(e,f,state); } \
            typedef stt transition_table; \
            typedef istate initial_state; \
            void setContext(CTX* const ctx) { m_context = ctx; } \
            CTX * const getContext() const { return m_context; } \
            name ## _() : m_context(0) {this->setStateMachineName(#name); this->setFsmName(#name);} \
        private: \
            CTX* m_context; \
        }; \
        typedef boost::msm::back::state_machine<name ## _<> > name;
#define KARABO_FSM_STATE_MACHINE_V_EE(name, stt, istate, context, entryFunc, exitFunc) \
_KARABO_FSM_STATE_MACHINE_IMPL_EE(name, stt, istate, context, entryFunc, exitFunc) \
virtual void entryFunc(); \
virtual void exitFunc();
#define KARABO_FSM_STATE_MACHINE_VE_EE(name, stt, istate, context, entryFunc, exitFunc) \
_KARABO_FSM_STATE_MACHINE_IMPL_EE(name, stt, istate, context, entryFunc, exitFunc) \
virtual void entryFunc() {} \
virtual void exitFunc() {}
#define KARABO_FSM_STATE_MACHINE_PV_EE(name, stt, istate, context, entryFunc, exitFunc) \
_KARABO_FSM_STATE_MACHINE_IMPL_EE(name, stt, istate, context, entryFunc, exitFunc) \
virtual void entryFunc() = 0; \
virtual void exitFunc()  = 0;

#define _KARABO_FSM_STATE_MACHINE_IMPL_AEE(name, stt, istate, context, TargetAction, entryFunc, exitFunc) \
        template<class CTX = context> \
        struct name ## _ : boost::msm::front::state_machine_def<name ## _<CTX>, karabo::core::FsmBaseState > { \
            typedef boost::msm::active_state_switch_after_transition_action active_state_switch_policy; \
            template <class Event, class Fsm> void on_entry(Event const& e, Fsm& f) { \
                try { \
                    /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
                    KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": entry"; \
                    f.getContext()->entryFunc(); \
                    _ta.setContext(f.getContext()); \
                    _worker->set(_ta, this->getTimeout(), this->getRepetition()).start(); \
                } catch(karabo::util::Exception const& e) { \
                    const std::string friendlyMsg(e.userFriendlyMsg(false)); \
                    _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
                } catch(...) { \
                    _onError<Fsm>(f); \
                }\
            } \
            template <class Event, class Fsm> void on_exit (Event const& e, Fsm& f) { \
                try { \
                    _worker->stop().join(); \
                    /* Context inherits from BaseFsm that inherits from SignalSlotable: */ \
                    KARABO_LOG_FRAMEWORK_DEBUG << f.getContext()->getInstanceId() << " " << #name << ": exit"; \
                    f.getContext()->exitFunc(); \
                } catch(karabo::util::Exception const& e) { \
                    const std::string friendlyMsg(e.userFriendlyMsg(false)); \
                    _onError<Fsm>(f, friendlyMsg, e.detailedMsg()); \
                } catch(...) { \
                    _onError<Fsm>(f); \
                }\
            } \
            template <class Fsm, class Event> void no_transition(Event const& e, Fsm& f, int state) { NoTransitionAction()(e,f,state); } \
            typedef stt transition_table; \
            typedef istate initial_state; \
            void setContext(CTX* const ctx) { m_context = ctx; } \
            CTX * const getContext() const { return m_context; } \
            name ## _() : _ta(), _worker(new Worker), m_context(0) {this->setStateMachineName(#name); this->setFsmName(#name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
            Worker* getWorker() const  {return _worker.get();} \
        private: \
            TargetAction _ta; \
            boost::shared_ptr<Worker> _worker; \
            CTX* m_context; \
        }; \
        typedef boost::msm::back::state_machine<name ## _<> > name;
#define KARABO_FSM_STATE_MACHINE_V_AEE(name, stt, istate, context, TargetAction, entryFunc, exitFunc) \
_KARABO_FSM_STATE_MACHINE_IMPL_AEE(name, stt, istate, context, TargetAction, entryFunc, exitFunc) \
virtual void entryFunc(); \
virtual void exitFunc();
#define KARABO_FSM_STATE_MACHINE_VE_AEE(name, stt, istate, context, TargetAction, entryFunc, exitFunc) \
_KARABO_FSM_STATE_MACHINE_IMPL_AEE(name, stt, istate, context, TargetAction, entryFunc, exitFunc) \
virtual void entryFunc() {} \
virtual void exitFunc() {}
#define KARABO_FSM_STATE_MACHINE_PV_AEE(name, stt, istate, context, TargetAction, entryFunc, exitFunc) \
_KARABO_FSM_STATE_MACHINE_IMPL_AEE(name, stt, istate, context, TargetAction, entryFunc, exitFunc) \
virtual void entryFunc() = 0; \
virtual void exitFunc()  = 0;

#endif

// clang-format on
