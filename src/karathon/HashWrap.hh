/* $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 16, 2012, 12:43 PM
 */

#ifndef KARABO_PYKARABO_HASHWRAP_HH
#define	KARABO_PYKARABO_HASHWRAP_HH

#include <boost/python.hpp>
#include <karabo/util/Hash.hh>
#include "Wrapper.hh"

namespace bp = boost::python;


namespace karabo {
    namespace pyexfel {


        class HashWrap {


            struct null_deleter {

                void operator()(void const *) const {
                }
            };

            static bool try_to_use_numpy;

            static void
            setPyListAsStdVector(karabo::util::Hash& self,
                                 const std::string& key,
                                 const bp::object& list,
                                 bp::ssize_t size,
                                 const char sep);

            static void
            setPyStrAsStdVector(karabo::util::Hash& self,
                                const std::string& key,
                                const bp::object& pystr,
                                const char sep);

        public:

            static const karabo::util::Hash&
            setPyDictAsHash(karabo::util::Hash& self,
                            const bp::dict& dictionary,
                            const char sep);

            static bp::object
            empty(const karabo::util::Hash & self);

            static void
            getKeys(const karabo::util::Hash & self,
                          const bp::object& obj);

            static bp::list
            keys(const karabo::util::Hash & self);

            static void
            getPaths(const karabo::util::Hash & self,
                           const bp::object& obj);

            static bp::list
            paths(const karabo::util::Hash & self);

            static bp::object
            getValues(const karabo::util::Hash& self);

            static bp::object
            get(const karabo::util::Hash& self,
                      const std::string& path,
                      const std::string& separator = ".");

            static bp::object
            getAs(const karabo::util::Hash& self,
                        const std::string& path,
                        const PyTypes::ReferenceType& type,
                        const std::string& separator = ".");

            static const karabo::util::Hash::Node&
            getNode(const karabo::util::Hash& self,
                          const std::string& path,
                          const std::string& separator = ".");

            static bp::object
            setNode(karabo::util::Hash& self,
                          const bp::object& node);

            static void
            set(karabo::util::Hash& self,
                      const std::string& key,
                      const bp::object & obj,
                      const std::string& separator = ".");

            static void
            erase(karabo::util::Hash& self,
                        const bp::object & keyObj,
                        const std::string& separator = ".");

            static bool
            has(karabo::util::Hash& self,
                      const std::string& key,
                      const std::string& separator = ".");

            static bool
            is(karabo::util::Hash& self,
                     const std::string& path,
                     const PyTypes::ReferenceType& type,
                     const std::string& separator = ".");

            static void
            flatten(const karabo::util::Hash& self,
                          karabo::util::Hash& flat,
                          const std::string& separator = ".");

            static void
            unflatten(const karabo::util::Hash& self,
                            karabo::util::Hash& tree,
                            const std::string& separator = ".");

            static bp::object
            getType(const karabo::util::Hash& self,
                          const std::string& path,
                          const std::string& separator = ".");

            static bool
            hasAttribute(karabo::util::Hash& self,
                               const std::string& path,
                               const std::string& attribute,
                               const std::string& separator = ".");

            static bp::object
            getAttribute(karabo::util::Hash& self,
                               const std::string& path,
                               const std::string& attribute,
                               const std::string& separator = ".");

            static bp::object
            getAttributeAs(karabo::util::Hash& self,
                                 const std::string& path,
                                 const std::string& attribute,
                                 const PyTypes::ReferenceType& type,
                                 const std::string& separator = ".");

            static const karabo::util::Hash::Attributes&
            getAttributes(karabo::util::Hash& self,
                                const std::string& path,
                                const std::string& separator = ".");

            static bp::object
            copyAttributes(karabo::util::Hash& self,
                                const std::string& path,
                                const std::string& separator = ".");

            static void
            setAttribute(karabo::util::Hash& self,
                               const std::string& path,
                               const std::string& attribute,
                               const bp::object& value,
                               const std::string& separator = ".");

            static void
            setAttributes(karabo::util::Hash& self,
                                const std::string& path,
                                const bp::object& attributes,
                                const std::string& separator = ".");

            static boost::shared_ptr<karabo::util::Hash::Node>
            find(karabo::util::Hash& self, const std::string& path,
                       const std::string& separator = ".");

            static bp::object
            __getitem__(karabo::util::Hash& self, const bp::object& obj);

            static void
            setDefault(const PyTypes::ReferenceType& type);

            static bool
            isDefault(const PyTypes::ReferenceType& type);

        };

        bool
        similarWrap(const bp::object& left, const bp::object& right);

    }
}

// Define 'bp::object' specialization for templated constructors of Hash class

namespace karabo {
    namespace util {

        template<>
        karabo::util::Hash::Hash(const std::string& key, const bp::object& value);

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2);

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3);

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3,
                                 const std::string& key4, const bp::object& value4);

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3,
                                 const std::string& key4, const bp::object& value4,
                                 const std::string& key5, const bp::object& value5);

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3,
                                 const std::string& key4, const bp::object& value4,
                                 const std::string& key5, const bp::object& value5,
                                 const std::string& key6, const bp::object& value6);
    }
}

#endif

