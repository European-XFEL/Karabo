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

        _KARABO_HELPER_MACRO(YOTTA, "Y")
        _KARABO_HELPER_MACRO(ZETTA, "Z")
        _KARABO_HELPER_MACRO(EXA, "E")
        _KARABO_HELPER_MACRO(PETA, "P")
        _KARABO_HELPER_MACRO(TERA, "T")
        _KARABO_HELPER_MACRO(GIGA, "G")
        _KARABO_HELPER_MACRO(MEGA, "M")
        _KARABO_HELPER_MACRO(KILO, "k")
        _KARABO_HELPER_MACRO(HECTO, "h")
        _KARABO_HELPER_MACRO(DECA, "da")
        template <> inline std::pair<std::string, std::string> Units::getMetricPrefix<Units::NONE>() {
            return std::make_pair("", "");
        }
        _KARABO_HELPER_MACRO(DECI, "d")
        _KARABO_HELPER_MACRO(CENTI, "c")
        _KARABO_HELPER_MACRO(MILLI, "m")
        _KARABO_HELPER_MACRO(MICRO, "u")
        _KARABO_HELPER_MACRO(NANO, "n")
        _KARABO_HELPER_MACRO(PICO, "p")
        _KARABO_HELPER_MACRO(FEMTO, "f")
        _KARABO_HELPER_MACRO(ATTO, "a")
        _KARABO_HELPER_MACRO(ZEPTO, "z")
        _KARABO_HELPER_MACRO(YOCTO, "y")

        #undef _KARABO_HELPER_MACRO


        #define _KARABO_HELPER_MACRO(unitEnum, symbol) \
         template <> inline std::pair<std::string, std::string> Units::getUnit<karabo::util::Units::unitEnum>() { std::string name(#unitEnum); boost::to_lower(name); return std::make_pair(name, symbol); }

        _KARABO_HELPER_MACRO(METER, "m")
        _KARABO_HELPER_MACRO(GRAM, "g")
        _KARABO_HELPER_MACRO(SECOND, "s")
        _KARABO_HELPER_MACRO(AMPERE, "A")
        _KARABO_HELPER_MACRO(KELVIN, "K")
        _KARABO_HELPER_MACRO(MOLE, "mol")
        _KARABO_HELPER_MACRO(CANDELA, "cd")
        _KARABO_HELPER_MACRO(HERTZ, "Hz")
        _KARABO_HELPER_MACRO(RADIAN, "rad")
        _KARABO_HELPER_MACRO(STERADIAN, "sr")
        _KARABO_HELPER_MACRO(NEWTON, "N")
        _KARABO_HELPER_MACRO(PASCAL, "Pa")
        _KARABO_HELPER_MACRO(JOULE, "J")
        _KARABO_HELPER_MACRO(WATT, "W")
        _KARABO_HELPER_MACRO(COULOMB, "C")
        _KARABO_HELPER_MACRO(VOLT, "V")
        _KARABO_HELPER_MACRO(FARAD, "F")
        _KARABO_HELPER_MACRO(OHM, "Ω")
        _KARABO_HELPER_MACRO(SIEMENS, "S")
        _KARABO_HELPER_MACRO(WEBER, "Wb")
        _KARABO_HELPER_MACRO(TESLA, "T")
        _KARABO_HELPER_MACRO(HENRY, "H")
        _KARABO_HELPER_MACRO(DEGREE_CELSIUS, "°C")
        _KARABO_HELPER_MACRO(LUMEN, "lm")
        _KARABO_HELPER_MACRO(LUX, "lx")
        _KARABO_HELPER_MACRO(BECQUEREL, "Bq")
        _KARABO_HELPER_MACRO(GRAY, "Gy")
        _KARABO_HELPER_MACRO(SIEVERT, "Sv")
        _KARABO_HELPER_MACRO(KATAL, "kat")
        _KARABO_HELPER_MACRO(MINUTE, "min")
        _KARABO_HELPER_MACRO(HOUR, "h")
        _KARABO_HELPER_MACRO(DAY, "d")
        _KARABO_HELPER_MACRO(YEAR, "a")
        _KARABO_HELPER_MACRO(BAR, "bar")
        _KARABO_HELPER_MACRO(PIXEL, "px")
        _KARABO_HELPER_MACRO(BYTE, "B")
        _KARABO_HELPER_MACRO(BIT, "B")

        #undef _KARABO_HELPER_MACRO
    }
}
#endif
