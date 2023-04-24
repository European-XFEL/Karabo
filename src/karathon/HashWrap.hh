/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/* $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 16, 2012, 12:43 PM
 */

#ifndef KARATHON_HASHWRAP_HH
#define KARATHON_HASHWRAP_HH

#include <boost/python.hpp>
#include <karabo/util/Hash.hh>

#include "Wrapper.hh"

namespace bp = boost::python;


namespace karathon {

    class HashWrap {
        struct null_deleter {
            void operator()(void const*) const {}
        };

        static bool try_to_use_numpy;

        static void setPyListAsStdVector(karabo::util::Hash& self, const std::string& key, const bp::object& list,
                                         bp::ssize_t size, const char sep);

        static void setPyStrAsStdVector(karabo::util::Hash& self, const std::string& key, const bp::object& pystr,
                                        const char sep);

       public:
        static const karabo::util::Hash& setPyDictAsHash(karabo::util::Hash& self, const bp::dict& dictionary,
                                                         const char sep);

        static bp::object empty(const karabo::util::Hash& self);

        static void getKeys(const karabo::util::Hash& self, const bp::object& obj);

        static bp::list keys(const karabo::util::Hash& self);

        static void getPaths(const karabo::util::Hash& self, const bp::object& obj);

        static bp::list paths(const karabo::util::Hash& self);

        static bp::object getValues(const karabo::util::Hash& self);

        static bp::object get(const karabo::util::Hash& self, const std::string& path,
                              const std::string& separator = ".", const bp::object& default_return = bp::object());

        static karabo::util::Hash copy(const karabo::util::Hash& self) {
            return self;
        }

        static bp::object getAs(const karabo::util::Hash& self, const std::string& path,
                                const PyTypes::ReferenceType& type, const std::string& separator = ".");

        static bp::object getNode(karabo::util::Hash& self, const std::string& path,
                                  const std::string& separator = ".");

        static bp::object setNode(karabo::util::Hash& self, const bp::object& node);

        static void set(karabo::util::Hash& self, const std::string& key, const bp::object& obj,
                        const std::string& separator = ".");

        static void setAs(karabo::util::Hash& self, const std::string& key, const bp::object& obj,
                          const bp::object& type, const std::string& separator = ".");

        static bool erase(karabo::util::Hash& self, const bp::object& keyObj, const std::string& separator = ".");

        static void erasePath(karabo::util::Hash& self, const bp::object& keyObj, const std::string& separator = ".");

        static bool has(karabo::util::Hash& self, const std::string& key, const std::string& separator = ".");

        static bool is(karabo::util::Hash& self, const std::string& path, const PyTypes::ReferenceType& type,
                       const std::string& separator = ".");

        static void flatten(const karabo::util::Hash& self, karabo::util::Hash& flat,
                            const std::string& separator = ".");

        static void unflatten(const karabo::util::Hash& self, karabo::util::Hash& tree,
                              const std::string& separator = ".");

        static bp::object getType(const karabo::util::Hash& self, const std::string& path,
                                  const std::string& separator = ".");

        static bool hasAttribute(karabo::util::Hash& self, const std::string& path, const std::string& attribute,
                                 const std::string& separator = ".");

        static bp::object getAttribute(karabo::util::Hash& self, const std::string& path, const std::string& attribute,
                                       const std::string& separator = ".");

        static bp::object getAttributeAs(karabo::util::Hash& self, const std::string& path,
                                         const std::string& attribute, const PyTypes::ReferenceType& type,
                                         const std::string& separator = ".");

        static const karabo::util::Hash::Attributes& getAttributes(karabo::util::Hash& self, const std::string& path,
                                                                   const std::string& separator = ".");

        static bp::object copyAttributes(karabo::util::Hash& self, const std::string& path,
                                         const std::string& separator = ".");

        static void setAttribute(karabo::util::Hash& self, const std::string& path, const std::string& attribute,
                                 const bp::object& value, const std::string& separator = ".");

        static void setAttributes(karabo::util::Hash& self, const std::string& path, const bp::object& attributes,
                                  const std::string& separator = ".");

        static void merge(karabo::util::Hash& self, const karabo::util::Hash& other,
                          const karabo::util::Hash::MergePolicy policy, const bp::object& selectedPaths,
                          const std::string& separator);

        static boost::shared_ptr<karabo::util::Hash::Node> find(karabo::util::Hash& self, const std::string& path,
                                                                const std::string& separator = ".");

        static bp::object getRef(karabo::util::Hash& self, const bp::object& obj, const std::string& sep = ".");

        static void setDefault(const PyTypes::ReferenceType& type);

        static bool isDefault(const PyTypes::ReferenceType& type);
    };

    bool similarWrap(const bp::object& left, const bp::object& right);

    bool fullyEqualWrap(const bp::object& left, const bp::object& right, const bp::object& orderMatters);

    // implementation details, users never invoke these directly
    namespace detail {

        inline void packPy_r(karabo::util::Hash& hash, char i) {}

        template <class Tfirst, class... Trest>
        inline void packPy_r(karabo::util::Hash& hash, char i, const Tfirst& first, const Trest&... rest) {
            char name[4] = "a ";
            name[1] = i;
            // Besides the following line, 'packPy_r' is identical to the C++ version 'karabo::util::pack_r'.
            HashWrap::set(hash, name, first);
            detail::packPy_r(hash, i + 1, rest...);
        }
    } // namespace detail

    /**
     * Pack the parameters into a hash for transport over the network.
     * @param hash Will be filled with keys a1, a2, etc. and associated values
     * @param args Any type and number of arguments to associated to hash keys
     */
    template <class... Ts>
    inline void packPy(karabo::util::Hash& hash, const Ts&... args) {
        detail::packPy_r(hash, '1', args...);
    }
} // namespace karathon

// Define 'bp::object' specialization for templated constructors of Hash class

namespace karabo {
    namespace util {

        template <>
        karabo::util::Hash::Hash(const std::string& key, const bp::object& value);

        template <>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1, const std::string& key2,
                                 const bp::object& value2);

        template <>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1, const std::string& key2,
                                 const bp::object& value2, const std::string& key3, const bp::object& value3);

        template <>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1, const std::string& key2,
                                 const bp::object& value2, const std::string& key3, const bp::object& value3,
                                 const std::string& key4, const bp::object& value4);

        template <>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1, const std::string& key2,
                                 const bp::object& value2, const std::string& key3, const bp::object& value3,
                                 const std::string& key4, const bp::object& value4, const std::string& key5,
                                 const bp::object& value5);

        template <>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1, const std::string& key2,
                                 const bp::object& value2, const std::string& key3, const bp::object& value3,
                                 const std::string& key4, const bp::object& value4, const std::string& key5,
                                 const bp::object& value5, const std::string& key6, const bp::object& value6);
    } // namespace util
} // namespace karabo

#endif /* KARATHON_HASHWRAP_HH */
