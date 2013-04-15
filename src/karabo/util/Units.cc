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
                    _KARABO_HELPER_MACRO(YOTTA)
                    _KARABO_HELPER_MACRO(ZETTA)
                    _KARABO_HELPER_MACRO(EXA)
                    _KARABO_HELPER_MACRO(PETA)
                    _KARABO_HELPER_MACRO(TERA)
                    _KARABO_HELPER_MACRO(GIGA)
                    _KARABO_HELPER_MACRO(MEGA)
                    _KARABO_HELPER_MACRO(KILO)
                    _KARABO_HELPER_MACRO(HECTO)
                    _KARABO_HELPER_MACRO(DECA)    
                    _KARABO_HELPER_MACRO(NONE)   
                    _KARABO_HELPER_MACRO(DECI)
                    _KARABO_HELPER_MACRO(CENTI)      
                    _KARABO_HELPER_MACRO(MILLI)
                    _KARABO_HELPER_MACRO(MICRO)
                    _KARABO_HELPER_MACRO(NANO)
                    _KARABO_HELPER_MACRO(PICO)
                    _KARABO_HELPER_MACRO(FEMTO)
                    _KARABO_HELPER_MACRO(ATTO)
                    _KARABO_HELPER_MACRO(ZEPTO)
                    _KARABO_HELPER_MACRO(YOCTO)
                default:
                    throw KARABO_PARAMETER_EXCEPTION("No string translation registered for given metricPrefix");
            }
            #undef _KARABO_HELPER_MACRO
        }


        std::pair<std::string, std::string> Units::getUnit(const Unit unit) {
            #define _KARABO_HELPER_MACRO(UnitEnum) case UnitEnum: return getUnit<UnitEnum>();
            switch (unit) {
                    _KARABO_HELPER_MACRO(METER)
                    _KARABO_HELPER_MACRO(GRAM)
                    _KARABO_HELPER_MACRO(SECOND)     
                    _KARABO_HELPER_MACRO(AMPERE)
                    _KARABO_HELPER_MACRO(KELVIN)                            
                    _KARABO_HELPER_MACRO(MOLE)
                    _KARABO_HELPER_MACRO(CANDELA)                            
                    _KARABO_HELPER_MACRO(HERTZ)
                    _KARABO_HELPER_MACRO(RADIAN)                           
                    _KARABO_HELPER_MACRO(STERADIAN)
                    _KARABO_HELPER_MACRO(NEWTON)
                    _KARABO_HELPER_MACRO(PASCAL)
                    _KARABO_HELPER_MACRO(JOULE)
                    _KARABO_HELPER_MACRO(WATT)
                    _KARABO_HELPER_MACRO(COULOMB)
                    _KARABO_HELPER_MACRO(VOLT)
                    _KARABO_HELPER_MACRO(FARAD)
                    _KARABO_HELPER_MACRO(OHM)
                    _KARABO_HELPER_MACRO(SIEMENS)
                    _KARABO_HELPER_MACRO(WEBER)
                    _KARABO_HELPER_MACRO(TESLA)
                    _KARABO_HELPER_MACRO(HENRY)
                    _KARABO_HELPER_MACRO(DEGREE_CELSIUS)
                    _KARABO_HELPER_MACRO(LUMEN)
                    _KARABO_HELPER_MACRO(LUX)
                    _KARABO_HELPER_MACRO(BECQUEREL)
                    _KARABO_HELPER_MACRO(GRAY)
                    _KARABO_HELPER_MACRO(SIEVERT)
                    _KARABO_HELPER_MACRO(KATAL)
                    _KARABO_HELPER_MACRO(MINUTE)
                    _KARABO_HELPER_MACRO(HOUR)
                    _KARABO_HELPER_MACRO(DAY)
                    _KARABO_HELPER_MACRO(YEAR)       
                    _KARABO_HELPER_MACRO(BAR)        
                    _KARABO_HELPER_MACRO(PIXEL)        
                    _KARABO_HELPER_MACRO(BYTE)        
                    _KARABO_HELPER_MACRO(BIT)      
                default:
                    throw KARABO_PARAMETER_EXCEPTION("No string translation registered for given unit");
            }
            #undef _KARABO_HELPER_MACRO
        }
    }
}