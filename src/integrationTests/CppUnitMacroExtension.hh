/*
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
/*
 * File:   CppUnitMacroExtension.hh
 * Author: flucke
 *
 * Created on February 12, 2021, 10:46 AM
 */

#ifndef CPPUNITMACROEXTENSION_HH
#define CPPUNITMACROEXTENSION_HH

#include <sstream>
#include <string>
#include <vector>

#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/StringTools.hh"

// Enable CPPUNIT_ASSERT_EQUAL for vectors and special case Hash and vector<Hash>.
// Special Hash treatment is needed because Hash::operator==(Hash) only compares paths
// (since using 'similarity').
// Also vector<float/double> might need more specialized treatment due to floating point comparison...
namespace CppUnit {

    template <>
    struct assertion_traits<karabo::data::Hash> {
        static bool equal(const karabo::data::Hash& a, const karabo::data::Hash& b) {
            // Unfortunately, Hash::operator==(const Hash&) just checks similarity...
            return a.fullyEquals(b);
        }

        static std::string toString(const karabo::data::Hash& p) {
            std::ostringstream o;
            o << p << std::endl;
            return o.str();
        }
    };


#ifndef NO_VECTOR_HASH_ASSERTION_TRAITS
    template <>
    struct assertion_traits<std::vector<karabo::data::Hash>> {
        static bool equal(const std::vector<karabo::data::Hash>& a, const std::vector<karabo::data::Hash>& b) {
            if (a.size() != b.size()) {
                return false;
            }
            for (size_t i = 0; i < a.size(); i++) {
                if (!a[i].fullyEquals(b[i])) {
                    return false;
                }
            }
            return true;
        }

        static std::string toString(const std::vector<karabo::data::Hash>& p) {
            std::ostringstream o;
            o << "(\n";
            for (size_t i = 0; i < p.size(); ++i) {
                o << '[' << i << "]:\n" << karabo::data::toString(p[i]);
            }
            o << ")";
            return o.str();
        }
    };
#endif // NO_VECTOR_HASH_ASSERTION_TRAITS

    template <>
    struct assertion_traits<std::vector<unsigned char>> {
        static bool equal(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) {
            return a == b;
        }

        static std::string toString(const std::vector<unsigned char>& p) {
            // Cannot use 'return karabo::data::toString(p)' since that uses base64 encoding
            std::ostringstream o;
            o << "'";
            for (const unsigned char& e : p) {
                o << static_cast<unsigned int>(e) << ',';
            }
            o << "'";
            return o.str();
        }
    };

    template <typename T>
    struct assertion_traits<std::vector<T>> {
        static bool equal(const std::vector<T>& a, const std::vector<T>& b) {
            return a == b;
        }

        static std::string toString(const std::vector<T>& p) {
            return karabo::data::toString(p);
        }
    };
} // namespace CppUnit


#endif /* CPPUNITMACROEXTENSION_HH */
