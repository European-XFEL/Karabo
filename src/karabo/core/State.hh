/* 
 * File:   BaseState.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on June 8, 2016, 2:49 PM
 */

#ifndef BASESTATE_HH
#define	BASESTATE_HH

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>
#include <karabo/util/ClassInfo.hh>
#include <karabo/util/Factory.hh>

namespace karabo {
    namespace core {

        namespace states {
            class StateSignifier;
        }

        /**
         * Base State class
         */
        class State {
            friend class states::StateSignifier;

        public:
            
            KARABO_CLASSINFO(State, "State", "1.0")

            State() : m_stateName(""), m_parents() {
            }

            State(const State& o) : m_stateName(o.m_stateName), m_parents(o.m_parents) {
            }

            virtual ~State() {
            }

            virtual const std::string& operator()() const {
                return m_stateName;
            }

            const std::string& name() const {
                return m_stateName;
            }
            
            void setStateName(const std::string& name) {
                m_stateName = name;
            }
            
            std::string parent() const {
                if (m_parents.empty()) return "";
                return m_parents[0];
            }
            
            const std::vector<std::string>& parents() const {
                return m_parents;
            }
            
            void setParent(const std::string& parent) {
                if (parent == "State") return;
                std::vector<std::string>::iterator it = m_parents.begin();
                it = m_parents.insert(it, parent);
            }
            
            void setParents(const std::vector<std::string>& parents) {
                for (size_t i=0; i < parents.size(); ++i)
                    m_parents.push_back(parents[i]);
            }

            State& operator=(const State& s) {
                m_stateName = s.m_stateName;
                m_parents = s.m_parents;
                return *this;
            }

            State& operator=(const std::string& stateName) {
                m_stateName = stateName;
                return *this;
            }

            bool isCompatible(const State& s) const {
                if (m_stateName == s.m_stateName) return true;
                for (size_t i = 0; i < m_parents.size(); i++)
                    if (m_parents[i] == s.m_stateName) return true;
                for (size_t ii = 0; ii < s.m_parents.size(); ++ii)
                    if (m_stateName == s.m_parents[ii]) return true;
                return false;
            }

            bool isCompatible(const std::string& s) const {
                if (m_stateName == s) return true;
                for (size_t i = 0; i < m_parents.size(); ++i)
                    if (m_parents[i] == s) return true;
                return false;
            }

        protected:
            std::string m_stateName;
            std::vector<std::string> m_parents;
            std::string m_fsmName;
            bool m_isContained;
            int m_timeout;
            int m_repetition;
        };

    }
}

#endif	/* BASESTATE_HH */

