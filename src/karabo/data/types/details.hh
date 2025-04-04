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
 * File:   details.hh (content moved from karabo/util/MetaTools.hh)
 *
 */

#ifndef KARABO_DATA_TYPES_DETAILS_HH
#define KARABO_DATA_TYPES_DETAILS_HH

#include <memory>
#include <type_traits>


namespace karabo::data {

    class Hash;

    namespace details { // A namespace not to be used outside namespace karabo

        template <class T>
        struct is_shared_ptr : std::false_type {};

        template <class T>
        struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

        /**
         * Conditionally cast a type to Hash, if Hash is a base class, but
         * disallow this for shared pointers of these types. Will result in
         * a compiler error if this is attempted.
         *
         * Any type not derived from Hash is simply returned as is.
         */
        template <typename is_hash_base>
        struct conditional_hash_cast {
            // Here is for T deriving from Hash or Hash itself,
            // below is the specialisation for is_hash_base = std::false_type.

            template <typename T>
            static const Hash& cast(const T& v) {
                return reinterpret_cast<const Hash&>(v);
            }

            template <typename T>
            static Hash&& cast(T&& v) {
                // Following line works fine in Ubuntu20 (gcc9.3), but does not compile on CentOS7gcc7 and Ubuntu18
                // (also gcc7) with "invalid cast of an rvalue expression of type ‘karabo::util::NDArray’ to type
                // ‘karabo::util::Hash&&’" from 'Hash::set(somekey, NDArray(..)) in PropertyTest::writeOutput(..).
                // return reinterpret_cast<Hash&&> (std::forward<T>(v));
                T&& vTemp = static_cast<T&&>(std::forward<T>(v));
                return reinterpret_cast<Hash&&>(vTemp);
            }

            template <typename T>
            static Hash& cast(T& v) {
                return reinterpret_cast<Hash&>(v);
            }

            static Hash& cast(Hash& v) {
                return v;
            }

            static Hash&& cast(Hash&& v) {
                return std::move(v);
            }

            static const Hash& cast(const Hash& v) {
                return v;
            }

            template <typename T>
            static const std::shared_ptr<T> cast(const std::shared_ptr<T>& v) {
                // if the compiler ever reaches this point compilation is to fail on purpose, as
                // we only support explicit setting of Hash::Pointer to the Hash

                // is_hash_base: will always be std::true_type when dealing
                // with types derived from Hash, i.e in the context of this
                // template method evaluation.
                static_assert(std::is_same<is_hash_base, std::false_type>::value, // this evaluates false
                              "Inserting derived hash classes as pointers is not supported");
                return v;
            }
        };


        template <>
        struct conditional_hash_cast<std::false_type> {
            template <typename T>
            static T&& cast(T&& v) {
                return std::forward<T>(v);
            }
        };

    } // namespace details
} // namespace karabo::data
#endif
