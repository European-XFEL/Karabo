/*
 * $Id: BobbyCar.hh 7033 2012-08-20 12:44:46Z coppola $
 *
 * File:   BobbyCar.hh
 * Author: <your.email@xfel.eu>
 *
 * Created on September 15, 2010, 12:38 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_BOBBYCAR_HH
#define	KARABO_UTIL_BOBBYCAR_HH

#include "Vehicle.hh"
#include <string>
#include <karabo/util/Schema.hh>

namespace karabo {

    namespace util {

        class BobbyCar : public Vehicle {

            public:

            KARABO_CLASSINFO(BobbyCar, "BobbyCar", "1.0")

            template <class Derived>
            BobbyCar(Derived* derived) : Vehicle() {
            }

            BobbyCar();

            void start();
            void stop();

            static void expectedParameters(Schema&);

            void configure(const Hash&);

        private:
            std::string m_name, m_equipment;

        };

    }
}

#endif
