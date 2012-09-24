/*
 * $Id$
 *
 * Author: esenov
 * Modified by: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

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

// Allow boost msm names appear globally in exfel namespace
namespace exfel {
    using boost::msm::front::none;
    using boost::msm::front::Row;
}

// Visitable polymorphic base state
#ifndef EXFEL_POLYMORPHIC_BASE_STATE
#define EXFEL_POLYMORPHIC_BASE_STATE
namespace exfel {

    namespace core {

        struct StateVisitor {

            template <class T>
            void visitState(T* state) {
                std::string stateName(state->getStateName());
                std::string fsmName(state->getFsmName());
                //std::cout << "visiting state:" << typeid (*state).name() << std::endl;
                // Technical correction: 
                // if state-machine and state are the same name, the state is subcomposed into the former machine
                if (stateName == fsmName) fsmName = m_currentFsm;
                
                if (m_stateName.empty()) {
                    m_stateName = stateName;
                    m_currentFsm = fsmName;
                } else {
                    std::string sep(".");
                    m_stateName += sep + stateName;
                    m_currentFsm = fsmName;
                }
            }

            const std::string& getState() {
                return m_stateName;
            }
            
        private:
            std::string m_stateName;
            std::string m_currentFsm;
        };

        struct FsmBaseState {
            
            const std::string & getStateName() const {
                return m_stateName;
            }
            
            virtual void setStateName(const std::string& name) {
                m_stateName = name;
            }
            
            const std::string& getFsmName() const {
                return m_fsmName;
            }
            
            virtual void setFsmName(const std::string& fsmName) {
                m_fsmName = fsmName;
            }
            
            const bool& isContained() const {
                return m_isContained;
            }
            
            void setContained(bool isContained) {
                m_isContained = isContained;
            }

            // Signature of the accept function
            typedef boost::msm::back::args<void, boost::shared_ptr<StateVisitor> > accept_sig;

            // This makes states polymorphic
            virtual ~FsmBaseState() {
            }

            // Default implementation for states who need to be visited

            void accept(boost::shared_ptr<StateVisitor> visitor) const {
                visitor->visitState(this);
            }

        private:
            std::string m_stateName;
            std::string m_fsmName;
            bool m_isContained;
        };
    }
}

#endif


/**************************************************************/
/*                    Special Macros                          */
/**************************************************************/

// Declares a shared pointer of a state machine
#define EXFEL_FSM_DECLARE_MACHINE(machineName, instanceName) boost::shared_ptr<machineName> instanceName; boost::recursive_mutex instanceName ## _mutex;

// Instantiates a state machine as a shared pointer
#define EXFEL_FSM_CREATE_MACHINE(machineName, instanceName) instanceName = boost::shared_ptr<machineName > (new machineName());

/**
 * Defines how logging information of the statemachine should be handled.
 * Internally produces an inline static function
 * 
 * @param logFunction A log function that must be implemented in the "context-class". The log function must be of signature:
 * The function must be of the following signature:
 * @code
 * LogStream& logFunction();
 * @endcode
 * @param Logstream The logstream that is returned AFTER the prefix is applied
 * @param prefix Any prefix that should be prepended to a log event
 */
#define EXFEL_FSM_LOGGER(logFunction, LogStream, prefix) \
template <class Fsm> \
static LogStream _log(const Fsm& fsm) { \
    return fsm.getContext()->logFunction() << prefix; \
}

#define EXFEL_FSM_TABLE_BEGIN(tableName) struct tableName : boost::mpl::vector< 

#define EXFEL_FSM_TABLE_END > {};

#define EXFEL_FSM_SET_CONTEXT_TOP(ctx, top) top->setContext(ctx);

#define EXFEL_FSM_SET_CONTEXT_SUB(ctx, parent, SubFsm) SubFsm* SubFsm##_ptr = parent->get_state<SubFsm* >(); SubFsm##_ptr->setContext(ctx);

#define EXFEL_FSM_SET_CONTEXT_SUB1(context, fsmInstance, SubFsm1) fsmInstance->get_state<SubFsm1* >()->setContext(context);

#define EXFEL_FSM_SET_CONTEXT_SUB2(context, fsmInstance, SubFsm1, SubFsm2) fsmInstance->get_state<SubFsm1* >()->get_state<SubFsm2* >()->setContext(context);

#define EXFEL_FSM_SET_CONTEXT_SUB3(context, fsmInstance, SubFsm1, SubFsm2, SubFsm3) fsmInstance->get_state<SubFsm1* >()->get_state<SubFsm2* >()->get_state<SubFsm3* >()->setContext(context);

// Produces a static function that calls errorFunction of the state machine's context
#define EXFEL_FSM_ON_EXCEPTION(errorFunction) \
template <class Fsm> \
static void _onError(const Fsm& fsm, const std::string& userFriendlyMsg = "Unknown error happened", const std::string& detailedMsg = "Unknown exception was triggered") { \
    fsm.getContext()->errorFunction(userFriendlyMsg, detailedMsg); \
}

// Produces a static function that calls stateChangeFunction of the state machine's context
#define EXFEL_FSM_ON_CURRENT_STATE_CHANGE(stateChangeFunction) \
virtual void stateChangeFunction(const std::string& currentState); \
template <class Fsm> \
static void _updateCurrentState(Fsm& fsm, bool isGoingToChange = false) { \
    if (isGoingToChange) fsm.getContext()->stateChangeFunction("Changing..."); \
    else { \
        boost::shared_ptr<StateVisitor> v(new StateVisitor); \
        fsm.visit_current_states(v); \
        fsm.getContext()->stateChangeFunction(v->getState()); \
    } \
}

#define EXFEL_FSM_START_MACHINE(machineInstance) machineInstance->start();_updateCurrentState(*machineInstance);

/**************************************************************/
/*                        Events                              */
/**************************************************************/

// 'm' - state machine pointer, 'name' - event name, 'f' - slot function, t1,t2,... - argument types
#define EXFEL_FSM_EVENT0(m,name,f) \
    struct name {};\
    void f() { boost::recursive_mutex::scoped_lock lock(m ## _mutex); _updateCurrentState(*m, true); m->process_event(name()); _updateCurrentState(*m); }

#define EXFEL_FSM_EVENT1(m,name,f,t1) \
    struct name { \
        name(const t1& b1) : a1(b1) {} \
        t1 a1; };\
    void f(const t1& c1) { boost::recursive_mutex::scoped_lock lock(m ## _mutex); _updateCurrentState(*m, true); m->process_event(name(c1));  _updateCurrentState(*m); }

#define EXFEL_FSM_EVENT2(m,name,f,t1,t2) \
    struct name { \
        name(const t1& b1,const t2& b2) : a1(b1),a2(b2) {}\
        t1 a1; \
        t2 a2; }; \
    void f(const t1& c1, const t2& c2) { boost::recursive_mutex::scoped_lock lock(m ## _mutex); _updateCurrentState(*m, true); m->process_event(name(c1,c2)); _updateCurrentState(*m);}

#define EXFEL_FSM_EVENT3(m,name,f,t1,t2,t3) \
    struct name { \
        name(const t1& b1,const t2& b2,const t3& b3) : a1(b1),a2(b2),a3(b3) {} \
        t1 a1; \
        t2 a2; \
        t3 a3; }; \
    void f(const t1& c1, const t2& c2, const t3& c3) { boost::recursive_mutex::scoped_lock lock(m ## _mutex); _updateCurrentState(*m, true); m->process_event(name(c1,c2,c3));_updateCurrentState(*m);}

#define EXFEL_FSM_EVENT4(m,name,f,t1,t2,t3,t4) \
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

#define _EXFEL_FSM_NO_TRANSITION_ACTION_IMPL(action) \
struct NoTransitionAction { \
    template <class Fsm, class Event> \
    void operator()(Event const& e, Fsm& f, int state) { \
        std::string type_id(typeid (e).name()); \
        _log<Fsm>(f) << #action; \
        f.getContext()->action(type_id,state); \
    } \
};

#define EXFEL_FSM_NO_TRANSITION_V_ACTION(action) \
_EXFEL_FSM_NO_TRANSITION_ACTION_IMPL(action);\
virtual void action(const std::string& typeId, int state);
#define EXFEL_FSM_NO_TRANSITION_PV_ACTION(action) \
_EXFEL_FSM_NO_TRANSITION_ACTION_IMPL(action) \
virtual void action(const std::string& typeId, int state) = 0;

#define _EXFEL_FSM_ACTION_IMPL0(name, func) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    void operator()(Evt const&, Fsm& f, SourceState&, TargetState&) { \
        try { \
            _log<Fsm>(f) << #name; \
            f.getContext()->func(); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define EXFEL_FSM_V_ACTION0(name, func) \
_EXFEL_FSM_ACTION_IMPL0(name, func) \
virtual void func();
#define EXFEL_FSM_PV_ACTION0(name, func) \
_EXFEL_FSM_ACTION_IMPL0(name, func) \
virtual void func() = 0;

#define _EXFEL_FSM_ACTION_IMPL1(name, func, t1) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    void operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            _log<Fsm>(f) << #name; \
            f.getContext()->func(e.a1); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define EXFEL_FSM_V_ACTION1(name, func, t1) \
_EXFEL_FSM_ACTION_IMPL1(name, func, t1) \
virtual void func(const t1&);
#define EXFEL_FSM_PV_ACTION1(name, func, t1) \
_EXFEL_FSM_ACTION_IMPL1(name, func, t1) \
virtual void func(const t1&)=0;

#define _EXFEL_FSM_ACTION_IMPL2(name, func, t1, t2) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    void operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            _log<Fsm>(f) << #name; \
            f.getContext()->func(static_cast<t1>(e.a1), static_cast<t2>(e.a2)); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define EXFEL_FSM_V_ACTION2(name, func, t1, t2) \
_EXFEL_FSM_ACTION_IMPL2(name, func, t1, t2) \
virtual void func(const t1&, const t2&);
#define EXFEL_FSM_PV_ACTION2(name, func, t1, t2) \
_EXFEL_FSM_ACTION_IMPL2(name, func, t1, t2) \
virtual void func(const t1&, const t2&) = 0;

#define _EXFEL_FSM_ACTION_IMPL3(name, func, t1, t2, t3) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    void operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            _log<Fsm>(f) << #name; \
            f.getContext()->func(static_cast<t1>(e.a1), static_cast<t2>(e.a2), static_cast<t3>(e.a3)); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define EXFEL_FSM_V_ACTION3(name, func, t1, t2, t3) \
_EXFEL_FSM_ACTION_IMPL3(name, func, t1, t2, t3) \
virtual void func(const t1&,const t2&,const t3&);
#define EXFEL_FSM_PV_ACTION3(name, func, t1, t2, t3) \
_EXFEL_FSM_ACTION_IMPL3(name, func, t1, t2, t3) \
virtual void func(const t1&,const t2&,const t3&) = 0;

#define _EXFEL_FSM_ACTION_IMPL4(name, func, t1, t2, t3, t4) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    void operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            _log<Fsm>(f) << #name; \
            f.getContext()->func(static_cast<t1>(e.a1), static_cast<t2>(e.a2), static_cast<t3>(e.a3), static_cast<t4>(e.a4)); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define EXFEL_FSM_V_ACTION4(name, func, t1, t2, t3, t4) \
_EXFEL_FSM_ACTION_IMPL4(name, func, t1, t2, t3, t4) \
virtual void func(const t1&,const t2&,const t3&,const t4&);
#define EXFEL_FSM_PV_ACTION4(name, func, t1, t2, t3, t4) \
_EXFEL_FSM_ACTION_IMPL4(name, func, t1, t2, t3, t4) \
virtual void func(const t1&,const t2&,const t3&,const t4&) = 0;

/**************************************************************/
/*                        Guards                              */
/**************************************************************/

#define _EXFEL_FSM_GUARD_IMPL0(name, func) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    bool operator()(Evt const&, Fsm& f, SourceState&, TargetState&) { \
        try { \
            _log<Fsm>(f) << #name; \
            return f.getContext()->func(); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
        return false; \
    } \
};
#define EXFEL_FSM_V_GUARD0(name, func) \
_EXFEL_FSM_GUARD_IMPL0(name, func) \
virtual bool func();
#define EXFEL_FSM_PV_GUARD0(name, func) \
_EXFEL_FSM_GUARD_IMPL0(name, func) \
virtual bool func() = 0;

#define _EXFEL_FSM_GUARD_IMPL1(name, func, t1) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    bool operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            _log<Fsm>(f) << #name; \
            return f.getContext()->func(static_cast<t1>(e.a1)); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
        return false; \
    } \
};
#define EXFEL_FSM_V_GUARD1(name, func, t1) \
_EXFEL_FSM_GUARD_IMPL1(name, func, t1) \
virtual bool func(const t1&);
#define EXFEL_FSM_PV_GUARD1(name, func, t1) \
_EXFEL_FSM_GUARD_IMPL1(name, func, t1) \
virtual bool func(const t1&) = 0;

#define _EXFEL_FSM_GUARD_IMPL2(name, func, t1, t2) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    bool operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            _log<Fsm>(f) << #name; \
            return f.getContext()->func(static_cast<t1>(e.a1), static_cast<t2>(e.a2)); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
        return false; \
    } \
};
#define EXFEL_FSM_V_GUARD2(name, func, t1, t2) \
_EXFEL_FSM_GUARD_IMPL2(name, func, t1, t2) \
virtual bool func(const t1&, const t2&);
#define EXFEL_FSM_PV_GUARD2(name, func, t1, t2) \
_EXFEL_FSM_GUARD_IMPL2(name, func, t1, t2) \
virtual bool func(const t1&, const t2&) = 0;

#define _EXFEL_FSM_GUARD_IMPL3(name, func, t1, t2, t3) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    bool operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            _log<Fsm>(f) << #name; \
            return f.getContext()->func(static_cast<t1>(e.a1), e.a2, e.a3); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
        return false; \
    } \
};
#define EXFEL_FSM_V_GUARD3(name, func, t1, t2, t3) \
_EXFEL_FSM_GUARD_IMPL3(name, func, t1, t2, t3) \
virtual bool func(const t1&, const t2&, const t3&);
#define EXFEL_FSM_PV_GUARD3(name, func, t1, t2, t3) \
_EXFEL_FSM_GUARD_IMPL3(name, func, t1, t2, t3) \
virtual bool func(const t1&, const t2&, const t3&) = 0;

#define _EXFEL_FSM_GUARD_IMPL4(name, func, t1, t2, t3, t4) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    bool operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            _log<Fsm>(f) << #name; \
            return f.getContext()->func(static_cast<t1>(e.a1), static_cast<t2>(e.a2), static_cast<t3>(e.a3), static_cast<t4>(e.a4)); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
        return false; \
    } \
};
#define EXFEL_FSM_V_GUARD4(name, func, a1, a2, a3, a4) \
_EXFEL_FSM_GUARD_IMPL4(name, func, t1, t2, t3, t4) \
virtual bool func(const t1&, const t2&, const t3&);
#define EXFEL_FSM_PV_GUARD4(name, func, a1, a2, a3, a4) \
_EXFEL_FSM_GUARD_IMPL4(name, func, t1, t2, t3, t4) \
virtual bool func(const t1&, const t2&, const t3&);

/**************************************************************/
/*                        States                              */
/**************************************************************/

#define EXFEL_FSM_STATE(name) struct name : public boost::msm::front::state<exfel::core::FsmBaseState> { \
name() {this->setStateName(#name);} \
template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
    try { \
        this->setFsmName(f.getFsmName()); \
        this->setContained(f.is_contained()); \
        _log<Fsm>(f) << #name << ": entry"; \
    } catch(exfel::util::Exception const& e) { \
        _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
    } catch(...) { \
        _onError<Fsm>(f); \
    }\
} \
};

#define _EXFEL_FSM_STATE_IMPL_E(name, entryFunc) \
struct name : public boost::msm::front::state<exfel::core::FsmBaseState> { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
             this->setContained(f.is_contained()); \
            _log<Fsm>(f) << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define EXFEL_FSM_STATE_V_E(name, entryFunc) \
_EXFEL_FSM_STATE_IMPL_E(name, entryFunc) \
virtual void entryFunc();
#define EXFEL_FSM_STATE_PV_E(name, entryFunc) \
_EXFEL_FSM_STATE_IMPL_E(name, entryFunc) \
virtual void entryFunc() = 0;

#define _EXFEL_FSM_STATE_IMPL_E1(name, entryFunc, event, t1) \
struct name : public boost::msm::front::state<exfel::core::FsmBaseState> { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(const Event& e, Fsm& f) {} \
    template <class Fsm> void on_entry(event const& e, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": entry"; \
            f.getContext()->entryFunc(static_cast<t1>(e.a1)); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};

#define EXFEL_FSM_STATE_V_E1(name, entryFunc, event, t1) \
_EXFEL_FSM_STATE_IMPL_E1(name, entryFunc, event, t1) \
virtual void entryFunc(const t1&);
#define EXFEL_FSM_STATE_PV_E1(name, entryFunc, event, t1) \
_EXFEL_FSM_STATE_IMPL_E1(name, entryFunc, event, t1) \
virtual void entryFunc(const t1&) = 0;

#define _EXFEL_FSM_STATE_IMPL_E2(name, entryFunc, t1, t2) \
struct name : public boost::msm::front::state<exfel::core::FsmBaseState> { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": entry"; \
            f.getContext()->entryFunc(static_cast<t1>(e.a1), static_cast<t2>(e.a2)); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define EXFEL_FSM_STATE_V_E2(name, entryFunc, t1, t2) \
_EXFEL_FSM_STATE_IMPL_E2(name, entryFunc, t1, t2) \
virtual void entryFunc(const t1&, const t2&);
#define EXFEL_FSM_STATE_PV_E2(name, entryFunc, t1, t2) \
_EXFEL_FSM_STATE_IMPL_E2(name, entryFunc, t1, t2) \
virtual void entryFunc(const t1&, const t2&) = 0;

#define _EXFEL_FSM_STATE_IMPL_E3(name, entryFunc, t1, t2, t3) \
struct name : public boost::msm::front::state<exfel::core::FsmBaseState> { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": entry"; \
            f.getContext()->entryFunc(static_cast<t1>(e.a1), static_cast<t2>(e.a2), static_cast<t3>(e.a3)); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    private: std::string m_state; \
};
#define EXFEL_FSM_STATE_V_E3(name, entryFunc, t1, t2, t3) \
_EXFEL_FSM_STATE_IMPL_E3(name, entryFunc, t1, t2, t3) \
virtual void entryFunc(const t1&, const t2&, const t3&);
#define EXFEL_FSM_STATE_PV_E3(name, entryFunc, t1, t2, t3) \
_EXFEL_FSM_STATE_IMPL_E3(name, entryFunc, t1, t2, t3) \
virtual void entryFunc(const t1&, const t2&, const t3&) = 0;


#define _EXFEL_FSM_STATE_IMPL_E4(name, entryFunc, t1, t2, t3, t4) \
struct name : public boost::msm::front::state<exfel::core::FsmBaseState> { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": entry"; \
            f.getContext()->entryFunc(static_cast<t1>(e.a1), static_cast<t2>(e.a2), static_cast<t3>(e.a3), static_cast<t4>(e.a4)); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define EXFEL_FSM_STATE_V_E4(name, entryFunc, t1, t2, t3, t4) \
_EXFEL_FSM_STATE_IMPL_E4(name, entryFunc, t1, t2, t3, t4) \
virtual void entryFunc(const t1&, const t2&, const t3&, const t4&);
#define EXFEL_FSM_STATE_PV_E4(name, entryFunc, t1, t2, t3, t4) \
_EXFEL_FSM_STATE_IMPL_E4(name, entryFunc, t1, t2, t3, t4) \
virtual void entryFunc(const t1&, const t2&, const t3& c3, const t4&) = 0;



#define _EXFEL_FSM_STATE_IMPL_EE(name, entryFunc, exitFunc) \
struct name : public boost::msm::front::state<exfel::core::FsmBaseState> { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": exit"; \
            f.getContext()->exitFunc(); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define EXFEL_FSM_STATE_V_EE(name, entryFunc, exitFunc) \
_EXFEL_FSM_STATE_IMPL_EE(name, entryFunc, exitFunc) \
virtual void entryFunc(); \
virtual void exitFunc();
#define EXFEL_FSM_STATE_PV_EE(name, entryFunc, exitFunc) \
_EXFEL_FSM_STATE_IMPL_EE(name, entryFunc, exitFunc) \
virtual void entryFunc() = 0; \
virtual void exitFunc()  = 0;

#define EXFEL_FSM_TERMINATE_STATE(name) \
struct name : public boost::msm::front::terminate_state<exfel::core::FsmBaseState> { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": entry"; \
            } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};


#define _EXFEL_FSM_TERMINATE_STATE_IMPL_E(name, entryFunc) \
struct name : public boost::msm::front::terminate_state<exfel::core::FsmBaseState> { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define EXFEL_FSM_TERMINATE_STATE_V_E(name, entryFunc) \
_EXFEL_FSM_TERMINATE_STATE_IMPL_E(name, entryFunc) \
virtual void entryFunc();
#define EXFEL_FSM_TERMINATE_STATE_PV_E(name, entryFunc) \
_EXFEL_FSM_TERMINATE_STATE_IMPL_E(name, entryFunc) \
virtual void entryFunc() = 0;


#define _EXFEL_FSM_TERMINATE_STATE_IMPL_EE(name, entryFunc, exitFunc) \
struct name : public boost::msm::front::terminate_state<exfel::core::FsmBaseState> { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": exit"; \
            f.getContext()->exitFunc();  \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define EXFEL_FSM_TERMINATE_STATE_V_EE(name, entryFunc, exitFunc) \
_EXFEL_FSM_TERMINATE_STATE_IMPL_EE(name, entryFunc, exitFunc) \
virtual void entryFunc(); \
virtual void exitFunc();
#define EXFEL_FSM_TERMINATE_STATE_PV_EE(name, entryFunc, exitFunc) \
_EXFEL_FSM_TERMINATE_STATE_IMPL_EE(name, entryFunc, exitFunc) \
virtual void entryFunc() = 0; \
virtual void exitFunc()  = 0;


#define EXFEL_FSM_INTERRUPT_STATE(name, event) \
struct name : public boost::msm::front::interrupt_state<event, exfel::core::FsmBaseState > { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": entry"; \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};


#define _EXFEL_FSM_INTERRUPT_STATE_IMPL_E(name, event, entryFunc) \
struct name : public boost::msm::front::interrupt_state<event, exfel::core::FsmBaseState > { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define EXFEL_FSM_INTERRUPT_STATE_V_E(name, event, entryFunc) \
_EXFEL_FSM_INTERRUPT_STATE_IMPL_E(name, event, entryFunc) \
virtual void entryFunc();
#define EXFEL_FSM_INTERRUPT_STATE_PV_E(name, event, entryFunc) \
_EXFEL_FSM_INTERRUPT_STATE_IMPL_E(name, event, entryFunc) \
virtual void entryFunc() = 0;


#define _EXFEL_FSM_INTERRUPT_STATE_IMPL_EE(name, event, entryFunc, exitFunc) \
struct name : public boost::msm::front::interrupt_state<event, exfel::core::FsmBaseState > { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": exit"; \
            f.getContext()->exitFunc(); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define EXFEL_FSM_INTERRUPT_STATE_V_EE(name, event, entryFunc, exitFunc) \
_EXFEL_FSM_INTERRUPT_STATE_IMPL_EE(name, event, entryFunc, exitFunc) \
virtual void entryFunc(); \
virtual void exitFunc();
#define EXFEL_FSM_INTERRUPT_STATE_PV_EE(name, event, entryFunc, exitFunc) \
_EXFEL_FSM_INTERRUPT_STATE_IMPL_EE(name, event, entryFunc, exitFunc) \
virtual void entryFunc() = 0; \
virtual void exitFunc()  = 0;


#define EXFEL_FSM_EXIT_PSEUDO_STATE(name, event) struct name : public boost::msm::front::exit_pseudo_state<event, exfel::core::FsmBaseState > { \
name() {this->setStateName(#name);} \
template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
    try { \
         this->setFsmName(f.getFsmName()); \
        _log<Fsm>(f) << #name << ": entry"; \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
} \
};

#define _EXFEL_FSM_EXIT_PSEUDO_STATE_IMPL_E(name, event, entryFunc) \
struct name : public boost::msm::front::exit_pseudo_state<event, exfel::core::FsmBaseState > { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define EXFEL_FSM_EXIT_PSEUDO_STATE_V_E(name, event, entryFunc) \
_EXFEL_FSM_EXIT_PSEUDO_STATE_IMPL_E(name, event, entryFunc) \
virtual void entryFunc();
#define EXFEL_FSM_EXIT_PSEUDO_STATE_PV_E(name, event, entryFunc) \
_EXFEL_FSM_EXIT_PSEUDO_STATE_IMPL_E(name, event, entryFunc) \
virtual void entryFunc() = 0;



#define _EXFEL_FSM_EXIT_PSEUDO_STATE_IMPL_EE(name, event, entryFunc, exitFunc) \
struct name : public boost::msm::front::exit_pseudo_state<event, exfel::core::FsmBaseState > { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
             this->setFsmName(f.getFsmName()); \
            _log<Fsm>(f) << #name << ": exit"; \
            f.getContext()->exitFunc(); \
        } catch(exfel::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};
#define EXFEL_FSM_EXIT_PSEUDO_STATE_V_EE(name, event, entryFunc, exitFunc) \
_EXFEL_FSM_EXIT_PSEUDO_STATE_IMPL_EE(name, event, entryFunc, exitFunc) \
virtual void entryFunc(); \
virtual void exitFunc();
#define EXFEL_FSM_EXIT_PSEUDO_STATE_PV_EE(name, event, entryFunc, exitFunc) \
_EXFEL_FSM_EXIT_PSEUDO_STATE_IMPL_EE(name, event, entryFunc, exitFunc) \
virtual void entryFunc() = 0; \
virtual void exitFunc()  = 0;

/**************************************************************/
/*                         Machines                           */
/**************************************************************/

#define EXFEL_FSM_REGION(x,y) boost::mpl::vector< x,y >

#define EXFEL_FSM_STATE_MACHINE(name, stt, istate, context) \
        template<class CTX = context> \
        struct name ## _ : boost::msm::front::state_machine_def<name ## _<CTX>, exfel::core::FsmBaseState > { \
            template <class Event, class Fsm> void on_entry(Event const& e, Fsm& f) { \
                try { \
                _log<Fsm>(f) << #name << ": entry"; \
                } catch(exfel::util::Exception const& e) { \
                _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
                } catch(...) { \
                _onError<Fsm>(f); \
                }\
            } \
            template <class Fsm, class Event> void no_transition(Event const& e, Fsm& f, int state) { NoTransitionAction()(e,f,state); } \
            typedef stt transition_table; \
            typedef istate initial_state; \
            void setContext(CTX* const ctx) { m_context = ctx; } \
            CTX * const getContext() const { return m_context; } \
            name ## _() : m_context(0) {this->setStateName(#name); this->setFsmName(#name);} \
        private: \
            CTX* m_context; \
        }; \
        typedef boost::msm::back::state_machine<name ## _<> > name;

#define _EXFEL_FSM_STATE_MACHINE_IMPL_E(name, stt, istate, context, entryFunc) \
        template<class CTX = context> \
        struct name ## _ : boost::msm::front::state_machine_def<name ## _<CTX>, exfel::core::FsmBaseState > { \
            template <class Event, class Fsm> void on_entry(Event const& e, Fsm& f) { \
                try { \
                    _log<Fsm>(f) << #name << ": entry"; \
                    f.getContext()->entryFunc(); \
                } catch(exfel::util::Exception const& e) { \
                _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
                } catch(...) { \
                _onError<Fsm>(f); \
                }\
            } \
            template <class Fsm, class Event> void no_transition(Event const& e, Fsm& f, int state) { NoTransitionAction()(e,f,state); } \
            typedef stt transition_table; \
            typedef istate initial_state; \
            void setContext(CTX* const ctx) { m_context = ctx; } \
            CTX * const getContext() const { return m_context; } \
            name ## _() : m_context(0) {this->setStateName(#name); this->setFsmName(#name);} \
        private: \
            CTX* m_context; \
        }; \
        typedef boost::msm::back::state_machine<name ## _<> > name;
#define EXFEL_FSM_STATE_MACHINE_V_E(name, stt, istate, context, entryFunc) \
_EXFEL_FSM_STATE_MACHINE_IMPL_E(name, stt, istate, context, entryFunc) \
virtual void entryFunc();
#define EXFEL_FSM_STATE_MACHINE_PV_E(name, stt, istate, context, entryFunc) \
_EXFEL_FSM_STATE_MACHINE_IMPL_E(name, stt, istate, context, entryFunc) \
virtual void entryFunc() = 0;


#define _EXFEL_FSM_STATE_MACHINE_IMPL_EE(name, stt, istate, context, entryFunc, exitFunc) \
        template<class CTX = context> \
        struct name ## _ : boost::msm::front::state_machine_def<name ## _<CTX>, exfel::core::FsmBaseState > { \
            template <class Event, class Fsm> void on_entry(Event const& e, Fsm& f) { \
                try { \
                    _log<Fsm>(f) << #name << ": entry"; \
                    f.getContext()->entryFunc(); \
                } catch(exfel::util::Exception const& e) { \
                _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
                } catch(...) { \
                _onError<Fsm>(f); \
                }\
                } \
            template <class Event, class Fsm> void on_exit (Event const& e, Fsm& f) { \
                try { \
                    _log<Fsm>(f) << #name << ": exit"; \
                    f.getContext()->exitFunc(); \
                } catch(exfel::util::Exception const& e) { \
                _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
                } catch(...) { \
                _onError<Fsm>(f); \
                }\
            } \
            template <class Fsm, class Event> void no_transition(Event const& e, Fsm& f, int state) { NoTransitionAction()(e,f,state); } \
            typedef stt transition_table; \
            typedef istate initial_state; \
            void setContext(CTX* const ctx) { m_context = ctx; } \
            CTX * const getContext() const { return m_context; } \
            name ## _() : m_context(0) {this->setStateName(#name); this->setFsmName(#name);} \
        private: \
            CTX* m_context; \
        }; \
        typedef boost::msm::back::state_machine<name ## _<> > name;
#define EXFEL_FSM_STATE_MACHINE_V_EE(name, stt, istate, context, entryFunc, exitFunc) \
_EXFEL_FSM_STATE_MACHINE_IMPL_EE(name, stt, istate, context, entryFunc, exitFunc) \
virtual void entryFunc(); \
virtual void exitFunc();
#define EXFEL_FSM_STATE_MACHINE_PV_EE(name, stt, istate, context, entryFunc, exitFunc) \
_EXFEL_FSM_STATE_MACHINE_IMPL_EE(name, stt, istate, context, entryFunc, exitFunc) \
virtual void entryFunc() = 0; \
virtual void exitFunc()  = 0;

#endif

