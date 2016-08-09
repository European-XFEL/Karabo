/*
 * $Id: AlarmTester.cc 7755 2016-06-24 14:10:56Z haufs $
 *
 * Author: <steffen.hauf@xfel.eu>
 * 
 * Created on June, 2016, 03:03 PM
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */

#include "AlarmTesterDevice.hh"
#include <cassert>

using namespace std;

USING_KARABO_NAMESPACES

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, AlarmTester)

    void AlarmTester::expectedParameters(Schema& expected) {
        
        FLOAT_ELEMENT(expected).key("floatProperty")
            .displayedName("Float Property")
            .readOnly().initialValue(0)
            .alarmLow(-2).info("A description for alarmLow").needsAcknowledging(true)
            .alarmHigh(2).info("A description for alarmHigh").needsAcknowledging(true)
            .enableRollingStats().warnVarianceHigh(3).needsAcknowledging(false).evaluationInterval(100)
            .commit();
        
        SLOT_ELEMENT(expected).key("triggerWarnLow")
            .displayedName("Trigger WARN_LOW")
            .commit();
        
        SLOT_ELEMENT(expected).key("triggerWarnHigh")
            .displayedName("Trigger WARN_HIGH")
            .commit();
        
        SLOT_ELEMENT(expected).key("triggerAlarmLow")
            .displayedName("Trigger ALARM_LOW")
            .commit();
        
        SLOT_ELEMENT(expected).key("triggerAlarmHigh")
            .displayedName("Trigger ALARM_HIGH")
            .commit();
        
        SLOT_ELEMENT(expected).key("triggerGlobalWarn")
            .displayedName("Trigger Global Warn")
            .commit();
        
        SLOT_ELEMENT(expected).key("triggerGlobalAlarm")
            .displayedName("Trigger Global Alarm")
            .commit();
        
        SLOT_ELEMENT(expected).key("triggerNormal")
            .displayedName("Back to normal")
            .commit();
        
        SLOT_ELEMENT(expected).key("triggerGlobalNormal")
            .displayedName("Back to global normal")
            .commit();
        
    }


    AlarmTester::AlarmTester(const karabo::util::Hash& config) : Device<>(config) {
        KARABO_SLOT(triggerWarnLow);
        KARABO_SLOT(triggerWarnHigh);
        KARABO_SLOT(triggerAlarmLow);
        KARABO_SLOT(triggerAlarmHigh);
        KARABO_SLOT(triggerGlobalAlarm);
        KARABO_SLOT(triggerGlobalWarn);
        KARABO_SLOT(triggerNormal);
        KARABO_SLOT(triggerGlobalNormal);
        
        KARABO_INITIAL_FUNCTION(initialize);
        
        
    }


    AlarmTester::~AlarmTester() {
    }

    void AlarmTester::initialize() {

        Schema schema =  this->getFullSchema();
        schema.setWarnLow("floatProperty", -1.);
        schema.setWarnHigh("floatProperty", 1.);
        schema.setAlarmLow("floatProperty", -2.);
        schema.setAlarmHigh("floatProperty", 2.);
        this->appendSchema(schema, true);

        
    }
    

    void AlarmTester::preReconfigure(karabo::util::Hash& incomingReconfiguration) {
    }


    void AlarmTester::postReconfigure() {
    }
    
    void AlarmTester::triggerWarnLow(){
        this->set("floatProperty", -1.2);
  
   
    }
    
    void AlarmTester::triggerWarnHigh(){
        this->set("floatProperty", 1.2);
        
   
    }
    
    void AlarmTester::triggerAlarmLow(){
        this->set("floatProperty", -2.2);
       
   
        KARABO_LOG_INFO<<this->getAlarmInfo();
    }
    
    void AlarmTester::triggerAlarmHigh(){
        this->set("floatProperty", 2.2);
       
   
        KARABO_LOG_INFO<<this->getAlarmInfo();
    }
    
    void AlarmTester::triggerGlobalAlarm(){
        this->setAlarmCondition(AlarmCondition::ALARM);
        
   
        KARABO_LOG_INFO<<this->getAlarmInfo();
    }
    
    void AlarmTester::triggerGlobalWarn(){
        this->setAlarmCondition(AlarmCondition::WARN);
        
   
    }

    void AlarmTester::triggerNormal(){
        this->set("floatProperty", 0);
        
   
        
    }
    
    void AlarmTester::triggerGlobalNormal(){
        this->setAlarmCondition(AlarmCondition::NONE);
       
   
    }
    
   
}
