/* 
 * File:   Units.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 29, 2013, 12:13 PM
 */

#ifndef KARABO_UTIL_UNITS_HH
#define	KARABO_UTIL_UNITS_HH

#include <boost/algorithm/string.hpp>

#include <karabo/util/karaboDll.hh>

#include "Exception.hh"

namespace karabo {
    namespace util {

        class Units {
        public:

            enum MetricPrefix {
                YOTTA,
                ZETTA,
                EXA,
                PETA,
                TERA,
                GIGA,
                MEGA,
                KILO,
                HECTO,
                DECA,
                NONE,
                DECI,
                CENTI,
                MILLI,
                MICRO,
                NANO,
                PICO,
                FEMTO,
                ATTO,
                ZEPTO,
                YOCTO
            };

            enum Unit {
                METER,
                GRAM,
                SECOND,
                AMPERE,
                KELVIN,
                MOLE,
                CANDELA,
                HERTZ,
                RADIAN,
                STERADIAN,
                NEWTON,
                PASCAL,
                JOULE,
                WATT,
                COULOMB,
                VOLT,
                FARAD,
                OHM,
                SIEMENS,
                WEBER,
                TESLA,
                HENRY,
                DEGREE_CELSIUS,
                LUMEN,
                LUX,
                BECQUEREL,
                GRAY,
                SIEVERT,
                KATAL,
                MINUTE,
                HOUR,
                DAY,
                WEEK,
                MONTH,
                YEAR,
                BAR,
                PIXEL,
                BYTE,
                BIT
            };

            template <int MetricEnum>
            static std::pair<std::string, std::string> getMetricPrefix() {
                throw KARABO_PARAMETER_EXCEPTION("");
            }

            template <int UnitEnum>
            static std::pair<std::string, std::string> getUnit() {
                throw KARABO_PARAMETER_EXCEPTION("");
            }

            static std::pair<std::string, std::string> getMetricPrefix(const MetricPrefix metricPrefix);

            static std::pair<std::string, std::string> getUnit(const Unit unit);


        };


        #define _KARABO_HELPER_MACRO(metricEnum, symbol) \
        template <> inline std::pair<std::string, std::string> Units::getMetricPrefix<karabo::util::Units::metricEnum>(){ std::string name(#metricEnum); boost::to_lower(name); return std::make_pair(name, symbol); }

        _KARABO_HELPER_MACRO(MILLI, "m")

        #undef _KARABO_HELPER_MACRO


        #define _KARABO_HELPER_MACRO(unitEnum, symbol) \
         template <> inline std::pair<std::string, std::string> Units::getUnit<karabo::util::Units::unitEnum>() { std::string name(#unitEnum); boost::to_lower(name); return std::make_pair(name, symbol); }

        _KARABO_HELPER_MACRO(METER, "m")

        #undef _KARABO_HELPER_MACRO
    }
}
#endif
