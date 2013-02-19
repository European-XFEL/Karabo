/* 
 * File:   Units.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 29, 2013, 12:13 PM
 */

#include "Units.hh"

namespace karabo {
    namespace util {

        std::pair<std::string, std::string> Units::getMetricPrefix(const MetricPrefix metricPrefix) {
            #define _KARABO_HELPER_MACRO(MetricEnum) case MetricEnum: return getMetricPrefix<MetricEnum>();
            switch (metricPrefix) {
                    _KARABO_HELPER_MACRO(MILLI)
                default:
                    throw KARABO_PARAMETER_EXCEPTION("No string translation registered for given metricPrefix");
            }
            #undef _KARABO_HELPER_MACRO
        }

        std::pair<std::string, std::string> Units::getUnit(const Unit unit) {
            #define _KARABO_HELPER_MACRO(UnitEnum) case UnitEnum: return getUnit<UnitEnum>();
            switch (unit) {
                    _KARABO_HELPER_MACRO(METER)
                default:
                    throw KARABO_PARAMETER_EXCEPTION("No string translation registered for given unit");
            }
            #undef _KARABO_HELPER_MACRO
        }
    }
}