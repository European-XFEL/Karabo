/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 1, 2011, 2:23 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_UTIL_MONITORABLEELEMENT_HH
#define	EXFEL_UTIL_MONITORABLEELEMENT_HH

#include "ComplexElement.hh"

namespace exfel {
  namespace util {

    template <class T>
    class MonitorableElement {
    private:

      ComplexElement m_outerElement;
      SimpleElement<T> m_actual, m_warnLow, m_warnHigh, m_alarmLow, m_alarmHigh;
      bool m_hasMax, m_hasMin, m_hasWarnLow, m_hasWarnHigh, m_hasAlarmLow, m_hasAlarmHigh;

    public:

      MonitorableElement(Schema& expected) : m_outerElement(ComplexElement(expected)),
                                             m_hasWarnLow(false), m_hasWarnHigh(false), 
                                             m_hasAlarmLow(false), m_hasAlarmHigh(false) {
        m_outerElement.reconfigurable();
        m_actual.key("actual");
        m_actual.displayedName("Actual Value");
        m_actual.readOnly().assignmentOptional().defaultValue("0");
      }

      MonitorableElement& key(const std::string& name) {
        m_outerElement.key(name);
        return *this;
      }

      MonitorableElement& displayedName(const std::string& displayedName) {
        m_outerElement.displayedName(displayedName);
        return *this;
      }

      MonitorableElement& description(const std::string& desc) {
        m_outerElement.description(desc);
        return *this;
      }

      MonitorableElement& unitName(const std::string& unitName) {
        m_actual.unitName(unitName);
        return *this;
      }

      MonitorableElement& unitSymbol(const std::string& unitSymbol) {
        m_actual.unitSymbol(unitSymbol);
        return *this;
      }
      
      template <class U>
      MonitorableElement& alias(const U& value) {
          m_actual.alias(value);
          return *this;
      }

      MonitorableElement& warnLow(const T& warnLow) {
        m_hasWarnLow = true;
        m_warnLow.key("warnLow");
        m_warnLow.displayedName("Warn Low");
        m_warnLow.description("If the actual value reaches or falls below this threshold a warning will be triggered");
        m_warnLow.assignmentOptional().defaultValue(warnLow);
        m_warnLow.reconfigurable();
        m_warnLow.advanced();
        return *this;
      }

      MonitorableElement& warnHigh(const T& warnHigh) {
        m_hasWarnHigh = true;
        m_warnHigh.key("warnHigh");
        m_warnHigh.displayedName("Warn High");
        m_warnHigh.description("If the actual value reaches of exceeds this threshold a warning will be triggered");
        m_warnHigh.assignmentOptional().defaultValue(warnHigh);
        m_warnHigh.reconfigurable();
        m_warnHigh.advanced();
        return *this;
      }

      MonitorableElement& alarmLow(const T& alarmLow) {
        m_hasAlarmLow = true;
        m_alarmLow.key("alarmLow");
        m_alarmLow.displayedName("Alarm Low");
        m_alarmLow.description("If the actual value reaches or falls below this threshold an alarm will be triggered");
        m_alarmLow.assignmentOptional().defaultValue(alarmLow);
        m_alarmLow.reconfigurable();
        m_alarmLow.advanced();
        return *this;
      }

      MonitorableElement& alarmHigh(const T& warnHigh) {
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
        m_actual.commit(innerElement);
        if (m_hasWarnLow) m_warnLow.commit(innerElement);
        if (m_hasWarnHigh) m_warnHigh.commit(innerElement);
        if (m_hasAlarmLow) m_alarmLow.commit(innerElement);
        if (m_hasAlarmHigh) m_alarmHigh.commit(innerElement);
      }
    };
    
    typedef MonitorableElement<bool > BOOL_MONITORABLE_ELEMENT;
    typedef MonitorableElement<signed char> INT8_MONITORABLE_ELEMENT;
    typedef MonitorableElement<char> CHAR_MONITORABLE_ELEMENT;
    typedef MonitorableElement<signed short> INT16_MONITORABLE_ELEMENT;
    typedef MonitorableElement<int> INT32_MONITORABLE_ELEMENT;
    typedef MonitorableElement<long long> INT64_MONITORABLE_ELEMENT;
    typedef MonitorableElement<unsigned char> UINT8_MONITORABLE_ELEMENT;
    typedef MonitorableElement<unsigned short> UINT16_MONITORABLE_ELEMENT;
    typedef MonitorableElement<unsigned int> UINT32_MONITORABLE_ELEMENT;
    typedef MonitorableElement<unsigned long long> UINT64_MONITORABLE_ELEMENT;
    typedef MonitorableElement<float> FLOAT_MONITORABLE_ELEMENT;
    typedef MonitorableElement<double> DOUBLE_MONITORABLE_ELEMENT;
    typedef MonitorableElement<std::string> STRING_MONITORABLE_ELEMENT;
    typedef MonitorableElement<boost::filesystem::path> PATH_MONITORABLE_ELEMENT;
    typedef MonitorableElement<Schema> CONFIG_MONITORABLE_ELEMENT;
  }
}



#endif	

