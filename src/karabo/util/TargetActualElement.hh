/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 1, 2011, 2:23 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_TARGETACTUALELEMENT_HH
#define	KARABO_UTIL_TARGETACTUALELEMENT_HH

#include "ComplexElement.hh"

namespace karabo {
  namespace util {

    template <class T, class A = T>
    class TargetActualElement {
    private:

      ComplexElement m_outerElement;
      SimpleElement<T> m_target, m_maxInc, m_minInc;
      SimpleElement<A> m_actual, m_warnLow, m_warnHigh, m_alarmLow, m_alarmHigh;
      bool m_hasMax, m_hasMin, m_hasWarnLow, m_hasWarnHigh, m_hasAlarmLow, m_hasAlarmHigh;

    public:

      TargetActualElement(Schema& expected) : m_outerElement(ComplexElement(expected)),
      m_hasMax(false), m_hasMin(false), m_hasWarnLow(false), m_hasWarnHigh(false), m_hasAlarmLow(false), m_hasAlarmHigh(false) {
        m_outerElement.reconfigureAndRead();
        m_target.key("target");
        m_target.displayedName("Target Value");
        m_actual.key("actual");
        m_actual.displayedName("Actual Value");
        m_actual.readOnly().assignmentOptional().defaultValueFromString("0");
        
        // By default the target is reconfigurable
        targetIsReconfigurable();
      }

      TargetActualElement& key(const std::string& name) {
        m_outerElement.key(name);
        return *this;
      }

      TargetActualElement& displayedName(const std::string& displayedName) {
        m_outerElement.displayedName(displayedName);
        return *this;
      }

      TargetActualElement& description(const std::string& desc) {
        m_outerElement.description(desc);
        return *this;
      }

      TargetActualElement& unitName(const std::string& unitName) {
        m_target.unitName(unitName);
        m_actual.unitName(unitName);
        return *this;
      }

      TargetActualElement& unitSymbol(const std::string& unitSymbol) {
        m_target.unitSymbol(unitSymbol);
        m_actual.unitSymbol(unitSymbol);
        return *this;
      }

      TargetActualElement& targetAssignmentMandatory() {
        m_outerElement.assignmentMandatory();
        m_target.assignmentMandatory();
        return *this;
      }

      TargetActualElement& targetAssignmentOptional() {
        m_outerElement.assignmentOptional();
        return *this;
      }
      
      TargetActualElement& targetIsInitOnly() {
        m_outerElement.initAndRead();
        m_target.init();
        return *this;
      }
      
      TargetActualElement& targetIsReconfigurable() {
        m_outerElement.reconfigureAndRead();
        m_target.reconfigurable();
        return *this;
      }
      
      TargetActualElement& targetAllowedStates(const std::string& states, const std::string& sep = ",") {
          m_target.allowedStates(states, sep);
          return *this;
      }
      
      TargetActualElement& targetDefaultValue(const T& defaultValue) {
        m_target.assignmentOptional().defaultValue(defaultValue);
        return *this;
      }

      TargetActualElement& targetHardOptions(const std::string& options, const std::string& sep = " ,;") {
        m_target.options(options, sep);
        return *this;
      }

      TargetActualElement& targetHardMax(const T& value) {
        m_target.maxInc(value);
        return *this;
      }

      TargetActualElement& targetHardMin(const T& value) {
        m_target.minInc(value);
        return *this;
      }

      TargetActualElement& targetConfigurableMax(const T& value) {
        m_hasMax = true;
        m_maxInc.key("targetMax");
        m_maxInc.displayedName("Target Maximum Value");
        m_maxInc.description("The maximum (inclusive) value which is allowed to assign to the target variable");
        m_maxInc.assignmentOptional().defaultValue(value);
        m_maxInc.reconfigurable();
        m_maxInc.advanced();
        return *this;
      }

      TargetActualElement& targetConfigurableMin(const T& value) {
        m_hasMin = true;
        m_minInc.key("targetMin");
        m_minInc.displayedName("Target Minimum Value");
        m_minInc.description("The minimum (inclusive) value which is allowed to assign to the target variable");
        m_minInc.assignmentOptional().defaultValue(value);
        m_minInc.reconfigurable();
        m_minInc.advanced();
        return *this;
      }
      
      template <class U>
      TargetActualElement& targetAlias(const U& value) {
        m_target.alias(value);
        return *this;
      }
      
      template <class U>
      TargetActualElement& actualAlias(const U& value) {
        m_actual.alias(value);
        return *this;
      }

      TargetActualElement& actualWarnLow(const A& warnLow) {
        m_hasWarnLow = true;
        m_warnLow.key("warnLow");
        m_warnLow.displayedName("Warn Low");
        m_warnLow.description("If the actual value reaches or falls below this threshold a warning will be triggered");
        m_warnLow.assignmentOptional().defaultValue(warnLow);
        m_warnLow.reconfigurable();
        m_warnLow.advanced();
        return *this;
      }

      TargetActualElement& actualWarnHigh(const A& warnHigh) {
        m_hasWarnHigh = true;
        m_warnHigh.key("warnHigh");
        m_warnHigh.displayedName("Warn High");
        m_warnHigh.description("If the actual value reaches of exceeds this threshold a warning will be triggered");
        m_warnHigh.assignmentOptional().defaultValue(warnHigh);
        m_warnHigh.reconfigurable();
        m_warnHigh.advanced();
        return *this;
      }

      TargetActualElement& actualAlarmLow(const A& alarmLow) {
        m_hasAlarmLow = true;
        m_alarmLow.key("alarmLow");
        m_alarmLow.displayedName("Alarm Low");
        m_alarmLow.description("If the actual value reaches or falls below this threshold an alarm will be triggered");
        m_alarmLow.assignmentOptional().defaultValue(alarmLow);
        m_alarmLow.reconfigurable();
        m_alarmLow.advanced();
        return *this;
      }

      TargetActualElement& actualAlarmHigh(const A& warnHigh) {
        m_hasAlarmHigh = true;
        m_alarmHigh.key("alarmHigh");
        m_alarmHigh.displayedName("Alarm High");
        m_alarmHigh.description("If the actual value reaches of exceeds this threshold a warning will be triggered");
        m_alarmHigh.assignmentOptional().defaultValue(warnHigh);
        m_alarmHigh.reconfigurable();
        m_alarmHigh.advanced();
        return *this;
      }

      void commit() {
        Schema& innerElement = m_outerElement.commit();
        m_target.commit(innerElement);
        m_actual.commit(innerElement);
        if (m_hasMax) m_maxInc.commit(innerElement);
        if (m_hasMin) m_minInc.commit(innerElement);
        if (m_hasWarnLow) m_warnLow.commit(innerElement);
        if (m_hasWarnHigh) m_warnHigh.commit(innerElement);
        if (m_hasAlarmLow) m_alarmLow.commit(innerElement);
        if (m_hasAlarmHigh) m_alarmHigh.commit(innerElement);
      }
    };
    
    typedef TargetActualElement<bool > BOOL_TARGETACTUAL_ELEMENT;
    typedef TargetActualElement<signed char> INT8_TARGETACTUAL_ELEMENT;
    typedef TargetActualElement<signed short> INT16_TARGETACTUAL_ELEMENT;
    typedef TargetActualElement<int> INT32_TARGETACTUAL_ELEMENT;
    typedef TargetActualElement<long long> INT64_TARGETACTUAL_ELEMENT;
    typedef TargetActualElement<unsigned char> UINT8_TARGETACTUAL_ELEMENT;
    typedef TargetActualElement<unsigned short> UINT16_TARGETACTUAL_ELEMENT;
    typedef TargetActualElement<unsigned int> UINT32_TARGETACTUAL_ELEMENT;
    typedef TargetActualElement<unsigned long long> UINT64_TARGETACTUAL_ELEMENT;
    typedef TargetActualElement<float> FLOAT_TARGETACTUAL_ELEMENT;
    typedef TargetActualElement<double> DOUBLE_TARGETACTUAL_ELEMENT;
    typedef TargetActualElement<std::string> STRING_TARGETACTUAL_ELEMENT;
    typedef TargetActualElement<boost::filesystem::path> PATH_TARGETACTUAL_ELEMENT;
    typedef TargetActualElement<Schema> CONFIG_TARGET_ACTUAL;
  }
}

#endif	

