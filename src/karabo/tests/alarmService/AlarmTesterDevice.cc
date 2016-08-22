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
            .alarmLow(-1.5).info("A description for alarmLow").needsAcknowledging(true)
            .alarmHigh(1.5).info("A description for alarmHigh").needsAcknowledging(true)
            .enableRollingStats().warnVarianceHigh(3).needsAcknowledging(false).evaluationInterval(100)
            .commit();
        
        FLOAT_ELEMENT(expected).key("floatProperty2")
            .displayedName("Float Property2")
            .readOnly().initialValue(0)
            .warnLow(-2).info("A description for alarmLow").needsAcknowledging(true)
            .warnHigh(2).info("A description for alarmHigh").needsAcknowledging(true)
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
        KARABO_SLOT(triggerWarnHigh2);
        KARABO_SLOT(triggerAlarmLow);
        KARABO_SLOT(triggerAlarmHigh);
        KARABO_SLOT(triggerGlobalAlarm);
        KARABO_SLOT(triggerGlobalWarn);
        KARABO_SLOT(triggerNormal);
        KARABO_SLOT(triggerNormal2);
        KARABO_SLOT(triggerGlobalNormal);
        
        KARABO_INITIAL_FUNCTION(initialize);
        
        
    }


    AlarmTester::~AlarmTester() {
    }

    void AlarmTester::initialize() {

        Schema schema =  getFullSchema();
        schema.setWarnLow("floatProperty", -1.);
        schema.setWarnHigh("floatProperty", 1.);
        schema.setAlarmLow("floatProperty", -2.);
        schema.setAlarmHigh("floatProperty", 2.);
        appendSchema(schema, true);

        
    }
    

    void AlarmTester::preReconfigure(karabo::util::Hash& incomingReconfiguration) {
    }


    void AlarmTester::postReconfigure() {
    }
    
    void AlarmTester::triggerWarnLow(){
        set("floatProperty", -1.2);
        reply("triggeredWarnLow");
  
   
    }
    
    void AlarmTester::triggerWarnHigh(){
        set("floatProperty", 1.2);
        reply("triggeredWarnHigh");
   
    }
    
    void AlarmTester::triggerWarnHigh2(){
        set("floatProperty2", 2.2);
        reply("triggeredWarnHigh2");
   
    }
    
    void AlarmTester::triggerAlarmLow(){
        set("floatProperty", -2.2);
       
   
        KARABO_LOG_INFO<<getAlarmInfo();
        reply("triggeredAlarmLow");
    }
    
    void AlarmTester::triggerAlarmHigh(){
        set("floatProperty", 2.2);
       
   
        KARABO_LOG_INFO<<getAlarmInfo();
        reply("triggeredAlarmHigh");
    }
    
    void AlarmTester::triggerGlobalAlarm(){
        setAlarmCondition(AlarmCondition::ALARM);
        
   
        KARABO_LOG_INFO<<getAlarmInfo();
        reply("triggeredGlobalAlarm");
    }
    
    void AlarmTester::triggerGlobalWarn(){
        setAlarmCondition(AlarmCondition::WARN);
        reply("triggeredGlobalWarn");
   
    }

    void AlarmTester::triggerNormal(){
        set("floatProperty", 0);
        reply("triggeredNormal");
   
        
    }
    
    void AlarmTester::triggerNormal2(){
        set("floatProperty2", 0);
        reply("triggeredNormal2");
   
        
    }
    
    void AlarmTester::triggerGlobalNormal(){
        setAlarmCondition(AlarmCondition::NONE);
        reply("triggeredGlobalNormal");
   
    }
    
   
}
