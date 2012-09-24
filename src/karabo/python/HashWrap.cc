/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "HashWrap.hh"

namespace exfel {
    namespace util {

        template<>
        exfel::util::Hash::Hash(const std::string& key, const bp::object& value) {
            exfel::pyexfel::HashWrap::pythonSetFromPath(*this, key, value);
        }

        template<>
        exfel::util::Hash::Hash(const std::string& key1, const bp::object& value1, const std::string& key2, const bp::object& value2) {
            exfel::pyexfel::HashWrap::pythonSetFromPath(*this, key1, value1);
            exfel::pyexfel::HashWrap::pythonSetFromPath(*this, key2, value2);
        }

        template<>
        exfel::util::Hash::Hash(const std::string& key1, const bp::object& value1, const std::string& key2, const bp::object& value2,
                const std::string& key3, const bp::object& value3) {
            exfel::pyexfel::HashWrap::pythonSetFromPath(*this, key1, value1);
            exfel::pyexfel::HashWrap::pythonSetFromPath(*this, key2, value2);
            exfel::pyexfel::HashWrap::pythonSetFromPath(*this, key3, value3);
        }

        template<>
        exfel::util::Hash::Hash(const std::string& key1, const bp::object& value1, const std::string& key2, const bp::object& value2,
                const std::string& key3, const bp::object& value3, const std::string& key4, const bp::object& value4) {
            exfel::pyexfel::HashWrap::pythonSetFromPath(*this, key1, value1);
            exfel::pyexfel::HashWrap::pythonSetFromPath(*this, key2, value2);
            exfel::pyexfel::HashWrap::pythonSetFromPath(*this, key3, value3);
            exfel::pyexfel::HashWrap::pythonSetFromPath(*this, key4, value4);
        }
    }
}
