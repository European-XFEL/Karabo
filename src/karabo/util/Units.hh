/* 
 * File:   Units.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 29, 2013, 12:13 PM
 */

#ifndef KARABO_UTIL_UNITS_HH_hh
#define	KARABO_UTIL_UNITS_HH_hh


#include <karabo/util/karaboDll.hh>

namespace karabo {
    namespace util {
        
        namespace metricPrefix {
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
        }
        typedef metricPrefix::MetricPrefix MetricPrefix;
            
        // TODO m^2 m^3 !!
        namespace unit {
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
                PIXEL
            };
        }
        typedef unit::Unit Unit;
    }
}
#endif
