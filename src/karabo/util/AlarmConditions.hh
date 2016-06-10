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
                std::string conditionString;
                unsigned int rank;
                
                
                BaseAlarmCondition() : conditionString("UNDEFINED"), rank(0) {
                    
                };
                
                BaseAlarmCondition(std::string cs) : conditionString(cs), rank(0) {
                    
                };
                BaseAlarmCondition(std::string cs, unsigned int r) : conditionString(cs), rank(r){
                   
                };
                BaseAlarmCondition(std::string cs, unsigned int r, Pointer b) : conditionString(cs), rank(r), basetype(b){
                    
                };
                
                
            public:
                
                Pointer basetype;
                
                const std::string & asString() const {
                    return conditionString;
                }
                
                Pointer returnMoreSignificant(Pointer other) const {
                    if (other->rank > this->rank) {
                        return other;
                    } else {
                        return shared_from_this();
                    }
                }
                
                operator std::string() const {
                    return this->conditionString;
                }
                
                
                bool isSimilar (const Pointer test) const {
                    return test->rank == this->rank;
                }
                
                
                
            };
            
            #define KRB_ALARM_NO_BASE(name, rank) class name ## _T: public BaseAlarmCondition{\
                                            public:\
                                            name ## _T() : BaseAlarmCondition(std::string(#name), rank){};\
                                           };\
                                           static boost::shared_ptr<name ## _T> name(new name ## _T);
                                           
                                           
                                           
            #define KRB_ALARM_BASE(name, rank) class name ## _T: public BaseAlarmCondition{\
                                            public:\
                                            name ## _T() : BaseAlarmCondition(std::string(#name), rank){};\
                                            name ## _T(std::string bs): BaseAlarmCondition(bs, rank, boost::make_shared<name ## _T>()){};\
                                           };\
                                           static boost::shared_ptr<name ## _T> name(new name ## _T);
                                     
                                           
            #define KRB_ALARM_FROM_BASE(name, base) class name ## _T: public base ## _T {\
                                            public:\
                                            name ## _T(): base ## _T (std::string(#name)){};\
                                           };\
                                           static boost::shared_ptr<name ## _T> name(new name ## _T);
                                        

                                            
            KRB_ALARM_NO_BASE(NONE, 0);
            KRB_ALARM_BASE(WARN, 1);
            KRB_ALARM_FROM_BASE(WARN_LOW, WARN);
            KRB_ALARM_FROM_BASE(WARN_HIGH, WARN);
            KRB_ALARM_BASE(ALARM, 2);
            KRB_ALARM_FROM_BASE(ALARM_LOW, ALARM);
            KRB_ALARM_FROM_BASE(ALARM_HIGH, ALARM);
            KRB_ALARM_NO_BASE(INTERLOCK, 3);

            
            static Pointer returnMostSignificant(const std::vector<Pointer> & v){
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
            
           
            
            static Pointer fromString(const std::string & condition){
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

#define KARABO_ALARM_ATTR "alarmCondition"

#endif	/* ALARMCONDITIONS_HH */

