#include "AlarmConditions.hh"
#include "Exception.hh"
#include "Schema.hh" // for attribute definitions

namespace karabo {
    namespace util {
  
        AlarmCondition::AlarmCondition() : m_conditionString("UNDEFINED"), m_rank(0) {
            
        };

        AlarmCondition::AlarmCondition(std::string cs, unsigned int r) : m_conditionString(cs), m_rank(r){
            
        };

        AlarmCondition::AlarmCondition(std::string cs, const AlarmCondition & b) : m_conditionString(cs), m_rank(b.m_rank), m_base(boost::make_shared<AlarmCondition>(b)){
            
        };
        
        

        boost::shared_ptr<const AlarmCondition> AlarmCondition::getBase() const {
                return m_base;
        }

           

        const AlarmCondition & AlarmCondition::returnMoreSignificant(const AlarmCondition & other) const {
            if (other.m_rank > this->m_rank) {
                return other;
            } else {
                return *this;
            }
        }    
        
        
        const std::string & AlarmCondition::asString() const {
                return m_conditionString;
        }
        
        const std::string & AlarmCondition::asBaseString() const {
                return (m_base.get() ? m_base->asString() : this->asString());
        }
        
        AlarmCondition::operator std::string() const {
                return this->m_conditionString;
        }


        bool AlarmCondition::isSameCriticality (const AlarmCondition & test) const {
            return test.m_rank == this->m_rank;
        }
           
            
        AlarmCondition AlarmCondition::returnMostSignificant(const std::vector<AlarmCondition> & v){
            if(v.empty()) return NONE;
            AlarmCondition s = v[0];
            for(std::vector<AlarmCondition>::const_iterator i = v.begin(); i != v.end(); i++){
                s = i->returnMoreSignificant(s);
                if (s.isSameCriticality(INTERLOCK)) break; // can't go higher than this
            }
            if (s.getBase()) {
                return *(s.getBase());
            } else {
                return s;
            }

        }

        const AlarmCondition & AlarmCondition::fromString(const std::string & condition){
            
            if (m_alarmFactory.empty()){
                #define KRB_ALARM_INSERT(alarmType) m_alarmFactory.insert(std::pair<std::string, const AlarmCondition& >(std::string(#alarmType), AlarmCondition::alarmType));
                KRB_ALARM_INSERT(NONE)
                KRB_ALARM_INSERT(WARN)
                KRB_ALARM_INSERT(WARN_HIGH)
                KRB_ALARM_INSERT(WARN_LOW)
                KRB_ALARM_INSERT(WARN_VARIANCE_HIGH)
                KRB_ALARM_INSERT(WARN_VARIANCE_LOW)
                KRB_ALARM_INSERT(ALARM)
                KRB_ALARM_INSERT(ALARM_LOW)
                KRB_ALARM_INSERT(ALARM_HIGH)
                KRB_ALARM_INSERT(ALARM_VARIANCE_LOW)
                KRB_ALARM_INSERT(ALARM_VARIANCE_HIGH)
                KRB_ALARM_INSERT(INTERLOCK)
            }
            
            std::map<std::string, const AlarmCondition &>::const_iterator iter =   m_alarmFactory.find(condition);
            if(iter == m_alarmFactory.end()){
                throw KARABO_LOGIC_EXCEPTION("Alarm condition  "+condition+" does not exist!");
            } else {
                return iter->second;
            }
        }
        
        
       
        const AlarmCondition AlarmCondition::NONE("NONE", 0);
        const AlarmCondition AlarmCondition::WARN("WARN", 1);
        const AlarmCondition AlarmCondition::WARN_LOW("WARN_LOW", WARN);
        const AlarmCondition AlarmCondition::WARN_HIGH("WARN_HIGH", WARN);
        const AlarmCondition AlarmCondition::WARN_VARIANCE_LOW("WARN_VARIANCE_LOW", WARN);
        const AlarmCondition AlarmCondition::WARN_VARIANCE_HIGH("WARN_VARIANCE_HIGH", WARN);
        const AlarmCondition AlarmCondition::ALARM("ALARM", 2);
        const AlarmCondition AlarmCondition::ALARM_LOW("ALARM_LOW", ALARM);
        const AlarmCondition AlarmCondition::ALARM_HIGH("ALARM_HIGH", ALARM);
        const AlarmCondition AlarmCondition::ALARM_VARIANCE_LOW("ALARM_VARIANCE_LOW", ALARM);
        const AlarmCondition AlarmCondition::ALARM_VARIANCE_HIGH("ALARM_VARIANCE_HIGH", ALARM);
        const AlarmCondition AlarmCondition::INTERLOCK("INTERLOCK", 3);
       //interlock is assumed to always be the highest conditions and knowledge of this is used in returnMostSignificant
        
        std::map<std::string, const AlarmCondition & > AlarmCondition::m_alarmFactory = std::map<std::string, const AlarmCondition &> ();
        
    }
}