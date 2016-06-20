/* 
 * File:   AlarmConditions.hh
 * Author: haufs
 *
 * Created on June 9, 2016, 9:13 AM
 */

#ifndef ALARMCONDITIONS_HH
#define	ALARMCONDITIONS_HH

#include <vector>
#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>

namespace karabo {
    namespace util {
        namespace alarmConditions {
            
            class BaseAlarmCondition;
            
            typedef boost::shared_ptr<const BaseAlarmCondition> Pointer;

            class BaseAlarmCondition : public boost::enable_shared_from_this<BaseAlarmCondition>{
              
            protected:
                std::string m_conditionString;
                unsigned int m_rank;
               
                
                
            public:
                
                
                
                BaseAlarmCondition() : m_conditionString("UNDEFINED"), m_rank(0) {
                };
               
                BaseAlarmCondition(std::string cs, unsigned int r) : m_conditionString(cs), m_rank(r){
                };
                
                BaseAlarmCondition(std::string cs, Pointer b) : m_conditionString(cs), m_rank(b->m_rank), basetype(b){
                };
            
                
                
                Pointer basetype;
                
                const std::string & asString() const {
                    return m_conditionString;
                }
                
                Pointer returnMoreSignificant(Pointer other) const {
                    if (other->m_rank > this->m_rank) {
                        return other;
                    } else {
                        return shared_from_this();
                    }
                }
                
                operator std::string() const {
                    return this->m_conditionString;
                }
                
                
                bool isSimilar (const Pointer test) const {
                    return test->m_rank == this->m_rank;
                }
                
                
                
            };
            
            
            extern const Pointer NONE;
            extern const Pointer WARN;
            extern const Pointer WARN_LOW;
            extern const Pointer WARN_HIGH;
            extern const Pointer WARN_VARIANCE_LOW;
            extern const Pointer WARN_VARIANCE_HIGH;
            extern const Pointer ALARM;
            extern const Pointer ALARM_LOW;
            extern const Pointer ALARM_HIGH;
            extern const Pointer ALARM_VARIANCE_LOW;
            extern const Pointer ALARM_VARIANCE_HIGH;
            extern const Pointer INTERLOCK;
                                           
            
            Pointer returnMostSignificant(const std::vector<Pointer> & v);
            
            Pointer fromString(const std::string & condition);
            
        }
        
    }
    
}

#define KARABO_ALARM_ATTR "alarmCondition"

#endif	/* ALARMCONDITIONS_HH */

