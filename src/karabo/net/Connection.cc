/*
 * $Id: Connection.cc 3392 2011-04-28 12:49:18Z heisenb@DESY.DE $
 *
 * File:   Connection.cc
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on November 2, 2010, 9:47 AM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "Connection.hh"

#include "Channel.hh"
#include "karabo/data/schema/SimpleElement.hh"

namespace karabo {
    namespace net {

        using namespace karabo::data;


        void Connection::expectedParameters(Schema& expected) {
            STRING_ELEMENT(expected)
                  .key("serializationType")
                  .displayedName("Serialization Type")
                  .description("Decides whether the serialization type for objects will be binary or text")
                  .options(std::vector<std::string>({"text", "binary"}))
                  .assignmentOptional()
                  .defaultValue("binary")
                  .init()
                  .commit();
        }


        Connection::Connection(const karabo::data::Hash& input) {
            input.get("serializationType", m_serializationType);
        }


        Connection::~Connection() {}

    } // namespace net
} // namespace karabo
