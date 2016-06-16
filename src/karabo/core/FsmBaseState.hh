/* 
 * File:   FsmBaseState.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on June 8, 2016, 3:45 PM
 */

#ifndef FSMBASESTATE_HH
#define	FSMBASESTATE_HH

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
#include "State.hh"
#include "Worker.hh"

// Allow boost msm names appear globally in karabo namespace
namespace karabo {
    using boost::msm::front::none;
    using boost::msm::front::Row;
}

namespace karabo {
    namespace core {
        
        struct StateVisitor {

            template <class T>
            void visitState(T* state, bool stopWorker=false) {
                if (stopWorker) {
                    Worker* worker = state->getWorker();
                    if (worker && worker->is_running()) worker->abort().join();
                } else {
                    std::string stateName(state->getStateName());
                    std::string fsmName(state->getFsmName());
                    //std::cout << "visiting state:" << typeid (*state).name() << std::endl;
                    // Technical correction: 
                    // if state-machine and state are the same name, the state is subcomposed into the former machine
                    if (stateName == fsmName) fsmName = m_currentFsm;

                    m_stateName = stateName;
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
            
            FsmBaseState() : m_state(new State), m_fsmName(""), m_isContained(false), m_timeout(-1), m_repetition(-1) {}

            const State::Pointer& getState() const {
                return m_state;
            }
            
            const std::string & getStateName() const {
                if (m_state->name().empty())
                    return m_stateMachineName;
                return m_state->name();
            }

            const std::string & name() const {
                if (m_state->name().empty())
                    return m_stateMachineName;
                return m_state->name();
            }
            
            void setStateMachineName(const std::string& name) {
                m_stateMachineName = name;
            }
  
            std::string parent() const {
                return m_state->parent();
            }
            
            const std::vector<std::string>& parents() const {
                return m_state->parents();
            }
//        protected:
//            void setParent(const std::string& parent) {
//                m_state->setParent(parent);
//            }
//            
//            void setParents(const std::vector<std::string>& parents) {
//                m_state->setParents(parents);
//            }
        public:
//            FsmBaseState& operator=(const State& s) {
//                m_state->setStateName(s.name());
//                m_state->setParents(s.parents());
//                return *this;
//            }
//
//            FsmBaseState& operator=(const FsmBaseState& s) {
//                setStateName(name());
//                setParents(parents());
//                return *this;
//            }
            
            bool isCompatible(const State& s) const {
                return m_state->isCompatible(s);
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
            typedef boost::msm::back::args<void, boost::shared_ptr<StateVisitor>, bool > accept_sig;

            // This makes states polymorphic

            virtual ~FsmBaseState() {
            }

            // Default implementation for states who need to be visited

            void accept(boost::shared_ptr<StateVisitor> visitor, bool stopWorker) const {
                visitor->visitState(this, stopWorker);
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

            virtual Worker* getWorker() const {
                return NULL;
            }

        protected:
            
            void setState(const State::Pointer& state) {
                m_state = state;
            }
            
        protected:
            State::Pointer m_state;
            std::string m_stateMachineName;
            std::string m_fsmName;
            bool m_isContained;
            int m_timeout;
            int m_repetition;
        };
        
    }
}



#endif	/* FSMBASESTATE_HH */

