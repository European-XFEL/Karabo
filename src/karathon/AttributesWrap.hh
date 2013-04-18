/* 
 * File:   AttributesWrap.hh
 * Author: esenov
 *
 * Created on March 20, 2013, 10:15 AM
 */

#ifndef ATTRIBUTESWRAP_HH
#define	ATTRIBUTESWRAP_HH

#include <karabo/util/Hash.hh>
#include <karabo/util/Types.hh>
#include <karabo/util/FromLiteral.hh>
#include <karabo/util/ToLiteral.hh>


namespace karabo {
    namespace pyexfel {


        class AttributesWrap {
        public:

            static bool has(karabo::util::Hash::Attributes& self, const std::string& key) {
                return self.has(key);
            }

            static bool pythonIs(karabo::util::Hash::Attributes& self, const std::string& key, const std::string& type) {
                using namespace karabo::util;
                Types::ReferenceType reftype = Types::from<FromLiteral>(type);
                return self.is(key, reftype);
            }

            static void erase(karabo::util::Hash::Attributes& self, const std::string& key) {
                self.erase(key);
            }

            static bp::object size(karabo::util::Hash::Attributes& self) {
                return bp::object(self.size());
            }

            static bool empty(karabo::util::Hash::Attributes& self) {
                return self.empty();
            }

            static void pythonClear(karabo::util::Hash::Attributes& self) {
                self.clear();
            }

            static bp::object getNode(karabo::util::Hash::Attributes& self, const std::string& key) {
                return bp::object(self.getNode(key));
            }
            
            static bp::object get(karabo::util::Hash::Attributes& self, const std::string& key) {
                return Wrapper::toObject(self.getAny(key));
            }
            
            static bp::object getAs(karabo::util::Hash::Attributes& self, const std::string& key, const std::string& type) {
                using namespace karabo::util;
                Types::ReferenceType reftype = Types::from<FromLiteral>(type);
                switch (reftype) {
                    case Types::BOOL:
                        return bp::object(self.getAs<bool>(key));
                    case Types::CHAR:
                        return bp::object(self.getAs<char>(key));
                    case Types::INT8:
                        return bp::object(self.getAs<signed char>(key));
                    case Types::UINT8:
                        return bp::object(self.getAs<unsigned char>(key));
                    case Types::INT16:
                        return bp::object(self.getAs<short>(key));
                    case Types::UINT16:
                        return bp::object(self.getAs<unsigned short>(key));
                    case Types::INT32:
                        return bp::object(self.getAs<int>(key));
                    case Types::UINT32:
                        return bp::object(self.getAs<unsigned int>(key));
                    case Types::INT64:
                        return bp::object(self.getAs<long long>(key));
                    case Types::UINT64:
                        return bp::object(self.getAs<unsigned long long>(key));
                    case Types::FLOAT:
                        return bp::object(self.getAs<float>(key));
                    case Types::DOUBLE:
                        return bp::object(self.getAs<double>(key));
                    case Types::STRING:
                        return bp::object(self.getAs<std::string>(key));
                    case Types::VECTOR_BOOL:
                        return Wrapper::fromStdVectorToPyArray(self.getAs<bool, std::vector > (key));
                    case Types::VECTOR_CHAR:
                        return Wrapper::fromStdVectorToPyByteArray(self.getAs<char, std::vector>(key));
                    case Types::VECTOR_INT8:
                        return Wrapper::fromStdVectorToPyByteArray(self.getAs<signed char, std::vector > (key));
                    case Types::VECTOR_UINT8:
                        return Wrapper::fromStdVectorToPyByteArray(self.getAs<unsigned char, std::vector>(key));
                    case Types::VECTOR_INT16:
                        return Wrapper::fromStdVectorToPyArray(self.getAs<short, std::vector>(key));
                    case Types::VECTOR_UINT16:
                        return Wrapper::fromStdVectorToPyArray(self.getAs<unsigned short, std::vector>(key));
                    case Types::VECTOR_INT32:
                        return Wrapper::fromStdVectorToPyArray(self.getAs<int, std::vector>(key));
                    case Types::VECTOR_UINT32:
                        return Wrapper::fromStdVectorToPyArray(self.getAs<unsigned int, std::vector>(key));
                    case Types::VECTOR_INT64:
                        return Wrapper::fromStdVectorToPyArray(self.getAs<long long, std::vector>(key));
                    case Types::VECTOR_UINT64:
                        return Wrapper::fromStdVectorToPyArray(self.getAs<unsigned long long, std::vector>(key));
//                    case Types::HASH:
//                        return bp::object(self.getAs<karabo::util::Hash>(key));
//                    case Types::VECTOR_HASH:
//                        return Wrapper::fromStdVectorToPyList(self.getAs<karabo::util::Hash, std::vector>(key));
                    default:
                        break;
                }
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not yet supported");
            }
            
            static bp::object set(karabo::util::Hash::Attributes& self, const std::string& key, const bp::object& value) {
                boost::any any;
                Wrapper::toAny(value, any);
                return bp::object(self.set(key, any));
            }
            
            static bp::object find(karabo::util::Hash::Attributes& self, const std::string& key) {
                karabo::util::Hash::Attributes::const_map_iterator it = self.find(key);
                if (it == self.mend())
                    return bp::object();
                return bp::object(it);
            }
            
            static bp::object getIt(karabo::util::Hash::Attributes& self, const bp::object& obj) {
                if (bp::extract<karabo::util::Hash::Attributes::const_map_iterator>(obj).check()) {
                    karabo::util::Hash::Attributes::const_map_iterator it = bp::extract<karabo::util::Hash::Attributes::const_map_iterator>(obj);
                    return Wrapper::toObject(self.get<boost::any>(it));
                } else if (bp::extract<karabo::util::Hash::Attributes::map_iterator>(obj).check()) {
                    karabo::util::Hash::Attributes::map_iterator it = bp::extract<karabo::util::Hash::Attributes::map_iterator>(obj);
                    return Wrapper::toObject(self.get<boost::any>(it));
                }
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Python type cannot be converted to Hash::Attributes::map_iterator");
            }
        };
    }
}

#endif	/* ATTRIBUTESWRAP_HH */

