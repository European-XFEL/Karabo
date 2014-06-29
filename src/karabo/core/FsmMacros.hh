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

#include <karabo/log/Logger.hh>

// Allow boost msm names appear globally in karabo namespace
namespace karabo {
    using boost::msm::front::none;
    using boost::msm::front::Row;
}

// Visitable polymorphic base state
#ifndef KARABO_POLYMORPHIC_BASE_STATE
#define KARABO_POLYMORPHIC_BASE_STATE
namespace karabo {

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

            const std::string & getState() {
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

            const std::string & getFsmName() const {
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

            void setTimeout(int timeout) {
                m_timeout = timeout;
            }
            
            int getTimeout() const {
                return m_timeout;
            }
            
            void setRepetition(int cycles) {
                m_repetition = cycles;
            }
            
            int getRepetition() const {
                return m_repetition;
            }
            
        private:
            std::string m_stateName;
            std::string m_fsmName;
            bool m_isContained;
            int m_timeout;
            int m_repetition;
        };
    }
}

#endif


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
#define KARABO_FSM_GET_DECLARE(machineName, instanceName) boost::shared_ptr<machineName> getFsm() { return instanceName; }
#define KARABO_FSM_GET0() getFsm() 
#define KARABO_FSM_GET1(A)            KARABO_FSM_GET0()->get_state<A*>()
#define KARABO_FSM_GET2(A,B)          KARABO_FSM_GET1(A)->get_state<B*>()
#define KARABO_FSM_GET3(A,B,C)        KARABO_FSM_GET2(A,B)->get_state<C*>()
#define KARABO_FSM_GET4(A,B,C,D)      KARABO_FSM_GET3(A,B,C)->get_state<D*>()
#define KARABO_FSM_GET5(A,B,C,D,E)    KARABO_FSM_GET4(A,B,C,D)->get_state<E*>()
#define KARABO_FSM_GET6(A,B,C,D,E,F)  KARABO_FSM_GET5(A,B,C,D,E)->get_state<F*>() 
#define KARABO_FSM_GET(n,...) KARABO_FSM_GET ## n(__VA_ARGS__)

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
    if (isGoingToChange) fsm.getContext()->stateChangeFunction("Changing..."); \
    else { \
        boost::shared_ptr<StateVisitor> v(new StateVisitor); \
        fsm.visit_current_states(v); \
        fsm.getContext()->stateChangeFunction(v->getState()); \
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
        KARABO_LOG_FRAMEWORK_DEBUG << #action; \
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
            KARABO_LOG_FRAMEWORK_DEBUG << #name; \
            f.getContext()->func(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
            KARABO_LOG_FRAMEWORK_DEBUG << #name; \
            f.getContext()->func(e.a1); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
            KARABO_LOG_FRAMEWORK_DEBUG << #name; \
            f.getContext()->func(static_cast<t1>(e.a1), static_cast<t2>(e.a2)); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
            KARABO_LOG_FRAMEWORK_DEBUG << #name; \
            f.getContext()->func(static_cast<t1>(e.a1), static_cast<t2>(e.a2), static_cast<t3>(e.a3)); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
            KARABO_LOG_FRAMEWORK_DEBUG << #name; \
            f.getContext()->func(static_cast<t1>(e.a1), static_cast<t2>(e.a2), static_cast<t3>(e.a3), static_cast<t4>(e.a4)); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
            KARABO_LOG_FRAMEWORK_DEBUG << #name; \
            return f.getContext()->func(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
virtual bool func() {}
#define KARABO_FSM_PV_GUARD0(name, func) \
_KARABO_FSM_GUARD_IMPL0(name, func) \
virtual bool func() = 0;

#define _KARABO_FSM_GUARD_IMPL1(name, func, t1) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    bool operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            KARABO_LOG_FRAMEWORK_DEBUG << #name; \
            return f.getContext()->func(static_cast<t1>(e.a1)); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
virtual bool func(const t1&) {}
#define KARABO_FSM_PV_GUARD1(name, func, t1) \
_KARABO_FSM_GUARD_IMPL1(name, func, t1) \
virtual bool func(const t1&) = 0;

#define _KARABO_FSM_GUARD_IMPL2(name, func, t1, t2) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    bool operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            KARABO_LOG_FRAMEWORK_DEBUG << #name; \
            return f.getContext()->func(static_cast<t1>(e.a1), static_cast<t2>(e.a2)); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
virtual bool func(const t1&, const t2&) {}
#define KARABO_FSM_PV_GUARD2(name, func, t1, t2) \
_KARABO_FSM_GUARD_IMPL2(name, func, t1, t2) \
virtual bool func(const t1&, const t2&) = 0;

#define _KARABO_FSM_GUARD_IMPL3(name, func, t1, t2, t3) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    bool operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            KARABO_LOG_FRAMEWORK_DEBUG << #name; \
            return f.getContext()->func(static_cast<t1>(e.a1), e.a2, e.a3); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
virtual bool func(const t1&, const t2&, const t3&) {}
#define KARABO_FSM_PV_GUARD3(name, func, t1, t2, t3) \
_KARABO_FSM_GUARD_IMPL3(name, func, t1, t2, t3) \
virtual bool func(const t1&, const t2&, const t3&) = 0;

#define _KARABO_FSM_GUARD_IMPL4(name, func, t1, t2, t3, t4) \
struct name { \
    template<class Fsm, class Evt, class SourceState, class TargetState> \
    bool operator()(Evt const& e, Fsm& f, SourceState&, TargetState&) { \
        try { \
            KARABO_LOG_FRAMEWORK_DEBUG << #name; \
            return f.getContext()->func(static_cast<t1>(e.a1), static_cast<t2>(e.a2), static_cast<t3>(e.a3), static_cast<t4>(e.a4)); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
virtual bool func(const t1&, const t2&, const t3&) {}
#define KARABO_FSM_PV_GUARD4(name, func, a1, a2, a3, a4) \
_KARABO_FSM_GUARD_IMPL4(name, func, t1, t2, t3, t4) \
virtual bool func(const t1&, const t2&, const t3&);


/**************************************************************/
/*                 In State Action                            */
/**************************************************************/

// WARNING: The following macro may be used only in classes where KARABO_CLASSINFO is used.
// It relies on the fact that 'Self' is properly defined (typedef to the class type)

#define _KARABO_FSM_PERIODIC_ACTION_IMPL1(name, timeout, repetition, func) \
struct name { \
    name() : m_timeout(timeout), m_repetition(repetition) {} \
    void setContext(Self* const ctx) { m_context = ctx; } \
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
_KARABO_FSM_PERIODIC_ACTION_IMPL1(name, timeout, repetition, func) \
void func();
#define KARABO_FSM_V_PERIODIC_ACTION(name, timeout, repetition, func) \
_KARABO_FSM_PERIODIC_ACTION_IMPL1(name, timeout, repetition, func) \
virtual void func();
#define KARABO_FSM_VE_PERIODIC_ACTION(name, timeout, repetition, func) \
_KARABO_FSM_PERIODIC_ACTION_IMPL1(name, timeout, repetition, func) \
virtual void func() {}
#define KARABO_FSM_PV_PERIODIC_ACTION(name, timeout, repetition, func) \
_KARABO_FSM_PERIODIC_ACTION_IMPL1(name, timeout, repetition, func) \
virtual void func() = 0;


/**************************************************************/
/*                        States                              */
/**************************************************************/

#define KARABO_FSM_STATE(name) struct name : public boost::msm::front::state<karabo::core::FsmBaseState> { \
name() {this->setStateName(#name);} \
template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
    try { \
        this->setFsmName(f.getFsmName()); \
        this->setContained(f.is_contained()); \
        KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
    } catch(karabo::util::Exception const& e) { \
        _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
    } catch(...) { \
        _onError<Fsm>(f); \
    }\
} \
};

#define KARABO_FSM_STATE_A(name, TargetAction) \
struct name : public boost::msm::front::state<karabo::core::FsmBaseState> { \
    TargetAction _ta; \
    name() : _ta() {this->setStateName(#name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            this->setContained(f.is_contained()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            _ta.setContext(f.getContext()); \
            f.getContext()->getWorker().set(_ta, this->getTimeout(), this->getRepetition()).start(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": exit"; \
            f.getContext()->getWorker().stop().join(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};

#define _KARABO_FSM_STATE_IMPL_E(name, entryFunc) \
struct name : public boost::msm::front::state<karabo::core::FsmBaseState> { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
             this->setContained(f.is_contained()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
    name() : _ta() {this->setStateName(#name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            this->setContained(f.is_contained()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(); \
            _ta.setContext(f.getContext()); \
            f.getContext()->getWorker().set(_ta, this->getTimeout(), this->getRepetition()).start(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": exit"; \
            f.getContext()->getWorker().stop().join(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
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
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(const Event& e, Fsm& f) {} \
    template <class Fsm> void on_entry(event const& e, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(static_cast<t1>(e.a1)); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(static_cast<t1>(e.a1), static_cast<t2>(e.a2)); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(static_cast<t1>(e.a1), static_cast<t2>(e.a2), static_cast<t3>(e.a3)); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const& e, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(static_cast<t1>(e.a1), static_cast<t2>(e.a2), static_cast<t3>(e.a3), static_cast<t4>(e.a4)); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": exit"; \
            f.getContext()->exitFunc(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
    TargetAction _ta; \
    name() : _ta() {this->setStateName(#name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(); \
            _ta.setContext(f.getContext()); \
            f.getContext()->getWorker().set(_ta, this->getTimeout(), this->getRepetition()).start(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            f.getContext()->getWorker().stop().join(); \
            this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": exit"; \
            f.getContext()->exitFunc(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
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
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};


#define _KARABO_FSM_TERMINATE_STATE_IMPL_E(name, entryFunc) \
struct name : public boost::msm::front::terminate_state<karabo::core::FsmBaseState> { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": exit"; \
            f.getContext()->exitFunc();  \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};


#define KARABO_FSM_INTERRUPT_STATE_A(name, event, TargetAction) \
struct name : public boost::msm::front::interrupt_state<event, karabo::core::FsmBaseState > { \
    TargetAction _ta; \
    name() : _ta() {this->setStateName(#name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            _ta.setContext(f.getContext()); \
            f.getContext()->getWorker().set(_ta, this->getTimeout(), this->getRepetition()).start(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": exit"; \
            f.getContext()->getWorker().stop().join(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
};


#define _KARABO_FSM_INTERRUPT_STATE_IMPL_E(name, event, entryFunc) \
struct name : public boost::msm::front::interrupt_state<event, karabo::core::FsmBaseState > { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
    name() : _ta() {this->setStateName(#name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(); \
            _ta.setContext(f.getContext()); \
            f.getContext()->getWorker().set(_ta, this->getTimeout(), this->getRepetition()).start(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": exit"; \
            f.getContext()->getWorker().stop().join(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
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
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": exit"; \
            f.getContext()->exitFunc(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
    name() : _ta() {this->setStateName(#name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
            this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(); \
            _ta.setContext(f.getContext()); \
            f.getContext()->getWorker().set(_ta, this->getTimeout(), this->getRepetition()).start(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
            f.getContext()->getWorker().stop().join(); \
            this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": exit"; \
            f.getContext()->exitFunc(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
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
name() {this->setStateName(#name);} \
template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
    try { \
         this->setFsmName(f.getFsmName()); \
        KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
} \
};

#define _KARABO_FSM_EXIT_PSEUDO_STATE_IMPL_E(name, event, entryFunc) \
struct name : public boost::msm::front::exit_pseudo_state<event, karabo::core::FsmBaseState > { \
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
    name() {this->setStateName(#name);} \
    template <class Event, class Fsm> void on_entry(Event const&, Fsm & f) { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
            f.getContext()->entryFunc(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
        } catch(...) { \
            _onError<Fsm>(f); \
        }\
    } \
    template <class Event, class Fsm> void on_exit(Event const&, Fsm & f)  { \
        try { \
             this->setFsmName(f.getFsmName()); \
            KARABO_LOG_FRAMEWORK_DEBUG << #name << ": exit"; \
            f.getContext()->exitFunc(); \
        } catch(karabo::util::Exception const& e) { \
            _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
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
                KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
                } catch(karabo::util::Exception const& e) { \
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

#define KARABO_FSM_STATE_MACHINE_A(name, stt, istate, context, TargetAction) \
        template<class CTX = context> \
        struct name ## _ : boost::msm::front::state_machine_def<name ## _<CTX>, karabo::core::FsmBaseState > { \
            typedef boost::msm::active_state_switch_after_transition_action active_state_switch_policy; \
            TargetAction _ta; \
            template <class Event, class Fsm> void on_entry(Event const& e, Fsm& f) { \
                try { \
                KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
                _ta.setContext(f.getContext()); \
                f.getContext()->getWorker().set(_ta, this->getTimeout(), this->getRepetition()).start(); \
                } catch(karabo::util::Exception const& e) { \
                _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
                } catch(...) { \
                _onError<Fsm>(f); \
                }\
            } \
            template <class Event, class Fsm> void on_exit (Event const& e, Fsm& f) { \
                try { \
                    f.getContext()->getWorker().stop().join(); \
                    KARABO_LOG_FRAMEWORK_DEBUG << #name << ": exit"; \
                } catch(karabo::util::Exception const& e) { \
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
            name ## _() : _ta(), m_context(0) {this->setStateName(#name); this->setFsmName(#name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
        private: \
            CTX* m_context; \
        }; \
        typedef boost::msm::back::state_machine<name ## _<> > name;

#define _KARABO_FSM_STATE_MACHINE_IMPL_E(name, stt, istate, context, entryFunc) \
        template<class CTX = context> \
        struct name ## _ : boost::msm::front::state_machine_def<name ## _<CTX>, karabo::core::FsmBaseState > { \
            typedef boost::msm::active_state_switch_after_transition_action active_state_switch_policy; \
            template <class Event, class Fsm> void on_entry(Event const& e, Fsm& f) { \
                try { \
                    KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
                    f.getContext()->entryFunc(); \
                } catch(karabo::util::Exception const& e) { \
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
            TargetAction _ta; \
            template <class Event, class Fsm> void on_entry(Event const& e, Fsm& f) { \
                try { \
                    KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
                    f.getContext()->entryFunc(); \
                    _ta.setContext(f.getContext()); \
                    f.getContext()->getWorker().set(_ta, this->getTimeout(), this->getRepetition()).start(); \
                } catch(karabo::util::Exception const& e) { \
                _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
                } catch(...) { \
                _onError<Fsm>(f); \
                }\
            } \
            template <class Event, class Fsm> void on_exit (Event const& e, Fsm& f) { \
                try { \
                    f.getContext()->getWorker().stop().join(); \
                    KARABO_LOG_FRAMEWORK_DEBUG << #name << ": exit"; \
                } catch(karabo::util::Exception const& e) { \
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
            name ## _() : _ta(),m_context(0) {this->setStateName(#name); this->setFsmName(#name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
        private: \
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
                    KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
                    f.getContext()->entryFunc(); \
                } catch(karabo::util::Exception const& e) { \
                _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
                } catch(...) { \
                _onError<Fsm>(f); \
                }\
                } \
            template <class Event, class Fsm> void on_exit (Event const& e, Fsm& f) { \
                try { \
                    KARABO_LOG_FRAMEWORK_DEBUG << #name << ": exit"; \
                    f.getContext()->exitFunc(); \
                } catch(karabo::util::Exception const& e) { \
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
            TargetAction _ta; \
            template <class Event, class Fsm> void on_entry(Event const& e, Fsm& f) { \
                try { \
                    KARABO_LOG_FRAMEWORK_DEBUG << #name << ": entry"; \
                    f.getContext()->entryFunc(); \
                    _ta.setContext(f.getContext()); \
                    f.getContext()->getWorker().set(_ta, this->getTimeout(), this->getRepetition()).start(); \
                } catch(karabo::util::Exception const& e) { \
                _onError<Fsm>(f, e.userFriendlyMsg(), e.detailedMsg()); \
                } catch(...) { \
                _onError<Fsm>(f); \
                }\
                } \
            template <class Event, class Fsm> void on_exit (Event const& e, Fsm& f) { \
                try { \
                    f.getContext()->getWorker().stop().join(); \
                    KARABO_LOG_FRAMEWORK_DEBUG << #name << ": exit"; \
                    f.getContext()->exitFunc(); \
                } catch(karabo::util::Exception const& e) { \
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
            name ## _() : _ta(),m_context(0) {this->setStateName(#name); this->setFsmName(#name); this->setTimeout(_ta.getTimeout()); this->setRepetition(_ta.getRepetition());} \
        private: \
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

