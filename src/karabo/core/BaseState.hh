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
        class BaseState {
            friend class states::StateSignifier;

        public:
            
            KARABO_CLASSINFO(BaseState, "BaseState", "1.0")

            BaseState() : m_stateName(""), m_parentName(""), m_id(0) {
            }

            BaseState(const BaseState& o) : m_stateName(o.m_stateName), m_parentName(o.m_parentName) {
            }

            virtual ~BaseState() {
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

            void operator=(const BaseState& s) {
                m_stateName = s.m_stateName;
                m_parentName = s.m_parentName;
                m_id = s.m_id;
            }

            void operator=(const std::string& stateName) {
                m_stateName = stateName;
            }

            void operator=(const char* const stateName) {
                m_stateName = stateName;
            }

            bool operator==(const BaseState& s) const {
                return m_stateName == s.m_stateName || m_stateName == s.m_parentName || m_parentName == s.m_stateName;
            }

            bool operator==(const std::string& s) const {
                return m_stateName == s || m_parentName == s;
            }

            bool operator!=(const BaseState& s) const {
                return !(this->operator==(s));
            }

            bool operator!=(const std::string& s) const {
                return !(this->operator==(s));
            }

            bool operator<(const BaseState& s) const {
                return m_id < s.m_id;
            }

            bool operator<=(const BaseState& s) const {
                return m_id <= s.m_id;
            }

            bool operator>(const BaseState& s) const {
                return m_id > s.m_id;
            }

            bool operator>=(const BaseState& s) const {
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

