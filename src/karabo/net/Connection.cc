/*
 * $Id: Connection.cc 3392 2011-04-28 12:49:18Z heisenb@DESY.DE $
 *
 * File:   Connection.cc
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on November 2, 2010, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "Connection.hh"

#include <karabo/util/SimpleElement.hh>

#include "Channel.hh"

namespace karabo {
    namespace net {

        using namespace karabo::util;


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


        Connection::Connection(const karabo::util::Hash& input) {
            input.get("serializationType", m_serializationType);
        }


        Connection::~Connection() {}

    } // namespace net
} // namespace karabo
