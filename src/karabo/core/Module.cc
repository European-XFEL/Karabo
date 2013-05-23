/*
 * $Id: Module.cc 2811 2011-01-07 10:40:52Z wrona $
 *
 * File:   Module.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on October 6, 2010, 2:25 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "Module.hh"
#include <string>

namespace exfel {
    namespace core {

        using namespace std;


        const string Module::getName() const {
            return getClassInfo().getClassId();
        }

    } // namespace core
} // namespace exfel
