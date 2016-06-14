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
        }
    }
}