#include "AlarmConditions.hh"

namespace karabo {
    namespace util {
        namespace alarmConditions {
            
            #define KRB_ALARM_BASE(name, rank) const Pointer name(new BaseAlarmCondition(std::string(#name), rank));

            #define KRB_ALARM_FROM_BASE(name, base) const Pointer name(new BaseAlarmCondition(std::string(#name), base));

            KRB_ALARM_BASE(NONE, 0);
            KRB_ALARM_BASE(WARN, 1);
            KRB_ALARM_FROM_BASE(WARN_LOW, WARN);
            KRB_ALARM_FROM_BASE(WARN_HIGH, WARN);
            KRB_ALARM_FROM_BASE(WARN_VARIANCE_LOW, WARN);
            KRB_ALARM_FROM_BASE(WARN_VARIANCE_HIGH, WARN);
            KRB_ALARM_BASE(ALARM, 2);
            KRB_ALARM_FROM_BASE(ALARM_LOW, ALARM);
            KRB_ALARM_FROM_BASE(ALARM_HIGH, ALARM);
            KRB_ALARM_FROM_BASE(ALARM_VARIANCE_LOW, ALARM);
            KRB_ALARM_FROM_BASE(ALARM_VARIANCE_HIGH, ALARM);
            KRB_ALARM_BASE(INTERLOCK, 3);
            
            Pointer returnMostSignificant(const std::vector<Pointer> & v){
                if(v.size() == 0) return NONE;
                Pointer s = v[0];
                for(std::vector<Pointer>::const_iterator i = v.begin(); i != v.end(); i++){
                    s = (*i)->returnMoreSignificant(s);
                    if (s->isSimilar(INTERLOCK)) break; // can't go higher than this
                }
                if (s->basetype) {
                    return s->basetype;
                } else {
                    return s;
                }
               
            }
            
            Pointer fromString(const std::string & condition){
                static std::map<std::string, Pointer > alarmFactory;
                if (alarmFactory.empty()){
                    #define KRB_ALARM_INSERT(alarmType) alarmFactory.insert(std::pair<std::string, Pointer >(std::string(#alarmType), alarmType));
                    KRB_ALARM_INSERT(NONE)
                    KRB_ALARM_INSERT(WARN)
                    KRB_ALARM_INSERT(WARN_HIGH)
                    KRB_ALARM_INSERT(WARN_LOW)
                    KRB_ALARM_INSERT(ALARM)
                    KRB_ALARM_INSERT(ALARM_LOW)
                    KRB_ALARM_INSERT(ALARM_HIGH)
                    KRB_ALARM_INSERT(INTERLOCK)
                } 
                return alarmFactory[condition];
            }
        }
    }
}