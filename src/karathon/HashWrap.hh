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

namespace bp = boost::python;


namespace karabo {
    namespace pyexfel {


        class HashWrap {


            struct null_deleter {

                void operator()(void const *) const {
                }
            };

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
            pythonEmpty(const karabo::util::Hash & self);

            static void
            pythonGetKeys(const karabo::util::Hash & self,
                          const bp::object& obj);

            static bp::list
            pythonKeys(const karabo::util::Hash & self);

            static void
            pythonGetPaths(const karabo::util::Hash & self,
                           const bp::object& obj);

            static bp::list
            pythonPaths(const karabo::util::Hash & self);

            static bp::object
            pythonGetValues(const karabo::util::Hash& self);

            static bp::object
            pythonGet(const karabo::util::Hash& self,
                      const std::string& path,
                      const std::string& separator = ".");

            static bp::object
            pythonGetAs(const karabo::util::Hash& self,
                        const std::string& path,
                        const karabo::util::Types::ReferenceType& type,
                        const std::string& separator = ".");

            static bp::object
            pythonGetNode(const karabo::util::Hash& self,
                          const std::string& path,
                          const std::string& separator = ".");

            static bp::object
            pythonSetNode(karabo::util::Hash& self,
                          const bp::object& node);

            static void
            pythonSet(karabo::util::Hash& self,
                      const std::string& key,
                      const bp::object & obj,
                      const std::string& separator = ".");

            static void
            pythonErase(karabo::util::Hash& self,
                        const bp::object & keyObj,
                        const std::string& separator = ".");

            static bool
            pythonHas(karabo::util::Hash& self,
                      const std::string& key,
                      const std::string& separator = ".");

            static bool
            pythonIs(karabo::util::Hash& self,
                     const std::string& path,
                     const karabo::util::Types::ReferenceType& type,
                     const std::string& separator = ".");

            static void
            pythonFlatten(const karabo::util::Hash& self,
                          karabo::util::Hash& flat,
                          const std::string& separator = ".");

            static void
            pythonUnFlatten(const karabo::util::Hash& self,
                            karabo::util::Hash& tree,
                            const std::string& separator = ".");

            static bp::object
            pythonGetType(const karabo::util::Hash& self,
                          const std::string& path,
                          const std::string& separator = ".");

            static bp::object
            pythonGetTypeAsId(const karabo::util::Hash& self,
                              const std::string& path,
                              const std::string& separator = ".");

            static bool
            pythonHasAttribute(karabo::util::Hash& self,
                               const std::string& path,
                               const std::string& attribute,
                               const std::string& separator = ".");

            static bp::object
            pythonGetAttribute(karabo::util::Hash& self,
                               const std::string& path,
                               const std::string& attribute,
                               const std::string& separator = ".");

            static bp::object
            pythonGetAttributeAs(karabo::util::Hash& self,
                                 const std::string& path,
                                 const std::string& attribute,
                                 const karabo::util::Types::ReferenceType& type,
                                 const std::string& separator = ".");

            static bp::object
            pythonGetAttributes(karabo::util::Hash& self,
                                const std::string& path,
                                const std::string& separator = ".");

            static void
            pythonSetAttribute(karabo::util::Hash& self,
                               const std::string& path,
                               const std::string& attribute,
                               const bp::object& value,
                               const std::string& separator = ".");

            static void
            pythonSetAttributes(karabo::util::Hash& self,
                                const std::string& path,
                                const bp::object& attributes,
                                const std::string& separator = ".");

            static boost::shared_ptr<karabo::util::Hash::Node>
            pythonFind(karabo::util::Hash& self, const std::string& path,
                       const std::string& separator = ".");

            static bp::object
            __getitem__(karabo::util::Hash& self, const bp::object& obj);

        };

        bool
        pythonSimilar(const bp::object& left, const bp::object& right);
        
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

