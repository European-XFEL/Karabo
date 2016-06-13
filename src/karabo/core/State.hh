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

            State() : m_stateName(""), m_parentName(""), m_id(0) {
            }

            State(const State& o) : m_stateName(o.m_stateName), m_parentName(o.m_parentName) {
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
            
            const std::string& parent() const {
                return m_parentName;
            }
            
            void setParentName(const std::string& parent) {
                m_parentName = parent;
            }

            const size_t rank() const {
                return m_id;
            }

            State& operator=(const State& s) {
                m_stateName = s.m_stateName;
                m_parentName = s.m_parentName;
                m_id = s.m_id;
                return *this;
            }

            State& operator=(const std::string& stateName) {
                m_stateName = stateName;
                return *this;
            }

            bool isCompatible(const State& s) const {
                return m_stateName == s.m_stateName || m_stateName == s.m_parentName || m_parentName == s.m_stateName;
            }

            bool isCompatible(const std::string& s) const {
                return m_stateName == s || m_parentName == s;
            }

            bool operator<(const State& s) const {
                return m_id < s.m_id;
            }

            bool operator<=(const State& s) const {
                return m_id <= s.m_id;
            }

            bool operator>(const State& s) const {
                return m_id > s.m_id;
            }

            bool operator>=(const State& s) const {
                return m_id >= s.m_id;
            }

        protected:
            std::string m_stateName;
            std::string m_parentName;
            size_t m_id;
            std::string m_fsmName;
            bool m_isContained;
            int m_timeout;
            int m_repetition;
        };

    }
}

#endif	/* BASESTATE_HH */

