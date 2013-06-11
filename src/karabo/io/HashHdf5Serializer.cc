/*
 * $Id: Format.cc 9852 2013-05-26 21:52:12Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "HashHdf5Serializer.hh" 
#include <karabo/log/Logger.hh>
#include <karabo/util/SimpleElement.hh>


using namespace std;
using namespace karabo::util;


namespace karabo {
    namespace io {


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Hdf5Serializer, karabo::io::HashHdf5Serializer)


        ////////////////////////////////////


        void HashHdf5Serializer::serializeHash(const karabo::util::Hash& data, hid_t group) {

            // This hash does not have attributes
            // It is not rooted. Either top or element of vector<Hash>
            //KARABO_LOG_FRAMEWORK_TRACE_CF << "path: " << path << " keyPath: " << keyPath;
            for (karabo::util::Hash::const_iterator it = data.begin(); it != data.end(); ++it) {
                if (it->is<karabo::util::Hash > ()) {
                    serializeHashElement(*it, group);
                } else if (it->is<std::vector<karabo::util::Hash> >()) {
                    serializeVectorOfHashesElement(*it, group);
                } else {
                    serializeDataElement(*it, group);
                }
            }
        }


        void HashHdf5Serializer::serializeHashElement(const karabo::util::Hash::Node& el, hid_t group) {

            const karabo::util::Hash& h = el.getValue<karabo::util::Hash > ();
            const std::string& key = el.getKey();

            hid_t h5obj = H5Gcreate(group, key.c_str(), H5P_DEFAULT, m_gcpl, H5P_DEFAULT);
            KARABO_CHECK_HDF5_STATUS(h5obj);

            if (!el.getAttributes().empty()) {
                serializeAttributes(el, h5obj);
            }

            for (karabo::util::Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
                if (it->is<karabo::util::Hash > ()) {
                    serializeHashElement(*it, h5obj);
                } else if (it->is<std::vector<karabo::util::Hash> >()) {
                    serializeVectorOfHashesElement(*it, h5obj);
                } else {


                    serializeDataElement(*it, h5obj);
                }
            }
            KARABO_CHECK_HDF5_STATUS(H5Gclose(h5obj));
        }


        void HashHdf5Serializer::serializeVectorOfHashesElement(const karabo::util::Hash::Node& el, hid_t group) {

            const std::vector<karabo::util::Hash>& vec = el.getValue<std::vector<karabo::util::Hash> >();
            const std::string& key = el.getKey();
            hid_t newGroup = H5Gcreate(group, key.c_str(), H5P_DEFAULT, m_gcpl, H5P_DEFAULT);
            KARABO_CHECK_HDF5_STATUS(newGroup);
            Hash tmp;
            tmp.set("a", 0);
            karabo::util::Hash::Node& krbNode = tmp.getNode("a");
            //            krbNode.setAttribute("KRB_type", "VECTOR_HASH");
            krbNode.setAttribute("KRB_size", static_cast<unsigned long long> (vec.size()));

            for (size_t i = 0; i < vec.size(); ++i) {
                std::ostringstream oss1;
                oss1 << "[" << i << "]";
                std::string newKey = key + oss1.str();
                hid_t h5obj = H5Gcreate(group, newKey.c_str(), H5P_DEFAULT, m_gcpl, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(h5obj);
                if (i == 0) {
                    serializeAttributes(krbNode, h5obj);
                }
                serializeHash(vec[i], h5obj);
                KARABO_CHECK_HDF5_STATUS(H5Gclose(h5obj));
            }
            KARABO_CHECK_HDF5_STATUS(H5Gclose(newGroup));
        }


        void HashHdf5Serializer::serializeDataElement(const karabo::util::Hash::Node& el, hid_t group) {

            karabo::util::Types::ReferenceType t = el.getType();
            const std::string& key = el.getKey();

            switch (t) {
                case karabo::util::Types::CHAR: serializeNodeByte(el, group);
                    break;
                case karabo::util::Types::INT8: serializeNode<signed char>(el, group);
                    break;
                case karabo::util::Types::INT16: serializeNode<short>(el, group);
                    break;
                case karabo::util::Types::INT32: serializeNode<int>(el, group);
                    break;
                case karabo::util::Types::INT64: serializeNode<long long>(el, group);
                    break;
                case karabo::util::Types::UINT8: serializeNode<unsigned char>(el, group);
                    break;
                case karabo::util::Types::UINT16: serializeNode<unsigned short>(el, group);
                    break;
                case karabo::util::Types::UINT32: serializeNode<unsigned int>(el, group);
                    break;
                case karabo::util::Types::UINT64: serializeNode<unsigned long long>(el, group);
                    break;
                case karabo::util::Types::FLOAT: serializeNode<float>(el, group);
                    break;
                case karabo::util::Types::DOUBLE: serializeNode<double>(el, group);
                    break;
                case karabo::util::Types::STRING: serializeNodeString(el, group);
                    break;
                case karabo::util::Types::BOOL: serializeNodeBool(el, group);
                    break;
                case karabo::util::Types::COMPLEX_FLOAT: serializeNodeComplex<float>(el, group);
                    break;
                case karabo::util::Types::COMPLEX_DOUBLE: serializeNodeComplex<double>(el, group);
                    break;
                case karabo::util::Types::VECTOR_CHAR: serializeNodeSequenceByte(el, group);
                    break;
                case karabo::util::Types::VECTOR_INT8: serializeNodeSequence<signed char>(el, group);
                    break;
                case karabo::util::Types::VECTOR_INT16: serializeNodeSequence<short>(el, group);
                    break;
                case karabo::util::Types::VECTOR_INT32: serializeNodeSequence<int>(el, group);
                    break;
                case karabo::util::Types::VECTOR_INT64: serializeNodeSequence<long long>(el, group);
                    break;
                case karabo::util::Types::VECTOR_UINT8: serializeNodeSequence<unsigned char>(el, group);
                    break;
                case karabo::util::Types::VECTOR_UINT16: serializeNodeSequence<unsigned short>(el, group);
                    break;
                case karabo::util::Types::VECTOR_UINT32: serializeNodeSequence<unsigned int>(el, group);
                    break;
                case karabo::util::Types::VECTOR_UINT64: serializeNodeSequence<unsigned long long>(el, group);
                    break;
                case karabo::util::Types::VECTOR_FLOAT: serializeNodeSequence<float>(el, group);
                    break;
                case karabo::util::Types::VECTOR_DOUBLE: serializeNodeSequence<double>(el, group);
                    break;
                case karabo::util::Types::VECTOR_STRING: serializeNodeSequence<std::string>(el, group);
                    break;
                case karabo::util::Types::VECTOR_BOOL: serializeNodeSequenceBool(el, group);
                    break;
                case karabo::util::Types::VECTOR_COMPLEX_FLOAT: serializeNodeSequenceComplex<float>(el, group);
                    break;
                case karabo::util::Types::VECTOR_COMPLEX_DOUBLE: serializeNodeSequenceComplex<double>(el, group);
                    break;

                default:
                    throw KARABO_NOT_SUPPORTED_EXCEPTION("Type not supported for key " + key);
            }
        }


        void HashHdf5Serializer::serializeAttributes(const karabo::util::Hash::Node& el, hid_t h5obj) {
            const karabo::util::Hash::Attributes& attr = el.getAttributes();
            for (karabo::util::Hash::Attributes::const_iterator it = attr.begin(); it != attr.end(); ++it) {
                karabo::util::Types::ReferenceType t = it->getType();
                const std::string& key = it->getKey();
                switch (t) {

                    case karabo::util::Types::CHAR: writeSingleAttribute(h5obj, it->getValue<char>(), key);
                        break;
                    case karabo::util::Types::INT8: writeSingleAttribute(h5obj, it->getValue<signed char>(), key);
                        break;
                    case karabo::util::Types::INT16: writeSingleAttribute(h5obj, it->getValue<short>(), key);
                        break;
                    case karabo::util::Types::INT32: writeSingleAttribute(h5obj, it->getValue<int>(), key);
                        break;
                    case karabo::util::Types::INT64: writeSingleAttribute(h5obj, it->getValue<long long>(), key);
                        break;
                    case karabo::util::Types::UINT8: writeSingleAttribute(h5obj, it->getValue<unsigned char>(), key);
                        break;
                    case karabo::util::Types::UINT16: writeSingleAttribute(h5obj, it->getValue<unsigned short>(), key);
                        break;
                    case karabo::util::Types::UINT32: writeSingleAttribute(h5obj, it->getValue<unsigned int>(), key);
                        break;
                    case karabo::util::Types::UINT64: writeSingleAttribute(h5obj, it->getValue<unsigned long long>(), key);
                        break;
                    case karabo::util::Types::FLOAT: writeSingleAttribute(h5obj, it->getValue<float>(), key);
                        break;
                    case karabo::util::Types::DOUBLE: writeSingleAttribute(h5obj, it->getValue<double>(), key);
                        break;
                    case karabo::util::Types::STRING: writeSingleAttribute(h5obj, it->getValue<std::string>(), key);
                        break;
                    case karabo::util::Types::BOOL: writeSingleAttribute(h5obj, it->getValue<bool>(), key);
                        break;
                    case karabo::util::Types::COMPLEX_FLOAT: writeSingleAttribute(h5obj, it->getValue<std::complex<float> >(), key);
                        break;
                    case karabo::util::Types::COMPLEX_DOUBLE: writeSingleAttribute(h5obj, it->getValue<std::complex<double> >(), key);
                        break;
                    case karabo::util::Types::VECTOR_CHAR: writeSequenceAttribute(h5obj, it->getValue< std::vector<char> >(), key);
                        break;
                    case karabo::util::Types::VECTOR_INT8: writeSequenceAttribute(h5obj, it->getValue < std::vector<signed char> >(), key);
                        break;
                    case karabo::util::Types::VECTOR_INT16: writeSequenceAttribute(h5obj, it->getValue< std::vector<short> >(), key);
                        break;
                    case karabo::util::Types::VECTOR_INT32: writeSequenceAttribute(h5obj, it->getValue< std::vector<int> >(), key);
                        break;
                    case karabo::util::Types::VECTOR_INT64: writeSequenceAttribute(h5obj, it->getValue< std::vector<long long> >(), key);
                        break;
                    case karabo::util::Types::VECTOR_UINT8: writeSequenceAttribute(h5obj, it->getValue< std::vector<unsigned char> >(), key);
                        break;
                    case karabo::util::Types::VECTOR_UINT16: writeSequenceAttribute(h5obj, it->getValue< std::vector<unsigned short> >(), key);
                        break;
                    case karabo::util::Types::VECTOR_UINT32: writeSequenceAttribute(h5obj, it->getValue< std::vector<unsigned int> >(), key);
                        break;
                    case karabo::util::Types::VECTOR_UINT64: writeSequenceAttribute(h5obj, it->getValue< std::vector<unsigned long long> >(), key);
                        break;
                    case karabo::util::Types::VECTOR_FLOAT: writeSequenceAttribute(h5obj, it->getValue< std::vector<float> >(), key);
                        break;
                    case karabo::util::Types::VECTOR_DOUBLE: writeSequenceAttribute(h5obj, it->getValue< std::vector<double> >(), key);
                        break;
                    case karabo::util::Types::VECTOR_STRING: writeSequenceAttribute(h5obj, it->getValue< std::vector<std::string> >(), key);
                        break;
                    case karabo::util::Types::VECTOR_BOOL: writeSequenceAttribute(h5obj, it->getValue < std::vector<bool> >(), key);
                        break;
                    case karabo::util::Types::VECTOR_COMPLEX_FLOAT: writeSequenceAttribute(h5obj, it->getValue< std::vector< std::complex<float> > >(), key);
                        break;
                    case karabo::util::Types::VECTOR_COMPLEX_DOUBLE: writeSequenceAttribute(h5obj, it->getValue< std::vector< std::complex<double> > >(), key);
                        break;
                    default:
                        throw KARABO_NOT_SUPPORTED_EXCEPTION("Type not supported for key " + key);

                }


            }
        }


        void HashHdf5Serializer::serializeNodeString(const karabo::util::Hash::Node& node, hid_t group) {
            const std::string& key = node.getKey();
            try {
                const std::string& value = node.getValue<std::string>();
                const char* ptr = value.c_str();
                hsize_t size = value.length();
                hid_t stype = H5Tcopy(H5T_C_S1);
                KARABO_CHECK_HDF5_STATUS(H5Tset_size(stype, size + 1));
                hid_t dsId = H5Dcreate2(group, key.c_str(), stype, m_spaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dwrite(dsId, stype, m_spaceId, m_spaceId, H5P_DEFAULT, ptr));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stype));
                serializeAttributes(node, dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dclose(dsId));
            } catch (...) {


                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }
        }


        void HashHdf5Serializer::serializeNodeBool(const karabo::util::Hash::Node& node, hid_t group) {
            const std::string& key = node.getKey();
            try {
                const bool& value = node.getValue<bool>();
                unsigned char converted = boost::numeric_cast<unsigned char>(value);
                hid_t stid = karabo::io::h5::ScalarTypes::getHdf5StandardType<bool>();
                hid_t ntid = karabo::io::h5::ScalarTypes::getHdf5NativeType<bool>();
                hid_t dsId = H5Dcreate2(group, key.c_str(), stid, m_spaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dwrite(dsId, ntid, m_spaceId, m_spaceId, H5P_DEFAULT, &converted));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(ntid));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stid));
                serializeAttributes(node, dsId);
                writeSingleAttribute(dsId, 1, "KRB_bool");
                KARABO_CHECK_HDF5_STATUS(H5Dclose(dsId));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }
        }


        void HashHdf5Serializer::serializeNodeByte(const karabo::util::Hash::Node& node, hid_t group) {
            const std::string& key = node.getKey();
            try {
                const char value = node.getValue<char>();
                hid_t tid = H5Tcreate(H5T_OPAQUE, 1);
                KARABO_CHECK_HDF5_STATUS(tid);
                KARABO_CHECK_HDF5_STATUS(H5Tset_tag(tid, "CHAR"));

                hid_t dsId = H5Dcreate2(group, key.c_str(), tid, m_spaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dwrite(dsId, tid, m_spaceId, m_spaceId, H5P_DEFAULT, &value));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                serializeAttributes(node, dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dclose(dsId));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }
        }


        void HashHdf5Serializer::serializeNodeSequenceByte(const karabo::util::Hash::Node& node, hid_t group) {
            const std::string& key = node.getKey();
            try {
                const vector<char>& vec = node.getValue<vector<char> >();

                hid_t tid = H5Tcreate(H5T_OPAQUE, vec.size());
                KARABO_CHECK_HDF5_STATUS(tid);
                KARABO_CHECK_HDF5_STATUS(H5Tset_tag(tid, "VECTOR_CHAR"));

                //                hsize_t dims[] = {1};
                //                hid_t spaceId = H5Screate_simple(1, dims, NULL);
                //                KARABO_CHECK_HDF5_STATUS(spaceId);

                hid_t dsId = H5Dcreate2(group, key.c_str(), tid, m_spaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dwrite(dsId, tid, m_spaceId, m_spaceId, H5P_DEFAULT, &vec[0]));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                //                KARABO_CHECK_HDF5_STATUS(H5Sclose(spaceId));
                serializeAttributes(node, dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dclose(dsId));

                //                const vector<char>& vec = node.getValue<vector<char> >();
                //                hid_t stid = H5Tvlen_create (H5T_STD_I8LE);
                //                hid_t ntid = H5Tvlen_create (H5T_NATIVE_INT8);
                //                hvl_t vlb;
                //                vlb.p = (void*)(&vec[0]);
                //                vlb.len = vec.size();
                //                hid_t dsId = H5Dcreate2(group, key.c_str(), stid, m_spaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                //                KARABO_CHECK_HDF5_STATUS(dsId);
                //                KARABO_CHECK_HDF5_STATUS(H5Dwrite(dsId, ntid, m_spaceId, m_spaceId, H5P_DEFAULT, &vlb));
                //                KARABO_CHECK_HDF5_STATUS(H5Tclose(stid));                
                //                serializeAttributes(node, dsId);
                //                KARABO_CHECK_HDF5_STATUS(H5Dclose(dsId));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }
        }


        void HashHdf5Serializer::serializeNodeSequenceBool(const karabo::util::Hash::Node& node, hid_t group) {
            const std::string& key = node.getKey();
            try {
                const std::vector<bool>& value = node.getValue < std::vector<bool> >();
                hsize_t len = value.size();
                hsize_t dims[] = {len};
                hid_t spaceId = H5Screate_simple(1, dims, NULL);
                KARABO_CHECK_HDF5_STATUS(spaceId);
                hid_t stid = karabo::io::h5::ScalarTypes::getHdf5StandardType<bool>();
                hid_t ntid = karabo::io::h5::ScalarTypes::getHdf5NativeType<bool>();
                std::vector<unsigned char> converted(len, 0);
                for (size_t i = 0; i < len; ++i) {
                    converted[i] = boost::numeric_cast<unsigned char>(value[i]);
                }
                const unsigned char* ptr = &converted[0];
                hid_t dsId = H5Dcreate2(group, key.c_str(), stid, spaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dwrite(dsId, ntid, spaceId, spaceId, H5P_DEFAULT, ptr));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(ntid));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(spaceId));
                serializeAttributes(node, dsId);
                writeSingleAttribute(dsId, 1, "KRB_bool");
                KARABO_CHECK_HDF5_STATUS(H5Dclose(dsId));
            } catch (...) {


                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }
        }


        void HashHdf5Serializer::writeSingleAttribute(hid_t group, const std::string& value, const std::string & key) {
            try {
                const char* ptr = value.c_str();
                hsize_t size = value.length();
                hid_t stype = H5Tcopy(H5T_C_S1);
                KARABO_CHECK_HDF5_STATUS(H5Tset_size(stype, size + 1));
                hid_t dsId = H5Acreate2(group, key.c_str(), stype, m_spaceId, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Awrite(dsId, stype, ptr));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stype));
                KARABO_CHECK_HDF5_STATUS(H5Aclose(dsId));
            } catch (...) {


                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }
        }


        void HashHdf5Serializer::writeSingleAttribute(hid_t group, const bool& value, const std::string & key) {
            try {
                unsigned char converted = boost::numeric_cast<unsigned char>(value);
                hid_t stid = karabo::io::h5::ScalarTypes::getHdf5StandardType<bool>();
                hid_t ntid = karabo::io::h5::ScalarTypes::getHdf5NativeType<bool>();
                hid_t dsId = H5Acreate2(group, key.c_str(), stid, m_spaceId, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Awrite(dsId, ntid, &converted));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(ntid));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stid));
                KARABO_CHECK_HDF5_STATUS(H5Aclose(dsId));
            } catch (...) {


                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }
        }


        void HashHdf5Serializer::writeSingleAttribute(hid_t group, const std::complex<float>& value, const std::string & key) {
            try {
                hid_t stid = karabo::io::h5::ScalarTypes::getHdf5StandardType<float>();
                hid_t ntid = karabo::io::h5::ScalarTypes::getHdf5NativeType<float>();
                hsize_t dims[] = {2};
                hid_t spaceId = H5Screate_simple(1, dims, NULL);
                hid_t dsId = H5Acreate2(group, key.c_str(), stid, spaceId, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Awrite(dsId, ntid, &value));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(ntid));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(spaceId));
                KARABO_CHECK_HDF5_STATUS(H5Aclose(dsId));
            } catch (...) {


                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }
        }


        void HashHdf5Serializer::writeSingleAttribute(hid_t group, const std::complex<double>& value, const std::string & key) {
            try {
                hid_t stid = karabo::io::h5::ScalarTypes::getHdf5StandardType<double>();
                hid_t ntid = karabo::io::h5::ScalarTypes::getHdf5NativeType<double>();
                hsize_t dims[] = {2};
                hid_t spaceId = H5Screate_simple(1, dims, NULL);
                hid_t dsId = H5Acreate2(group, key.c_str(), stid, spaceId, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Awrite(dsId, ntid, &value));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(ntid));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(spaceId));
                KARABO_CHECK_HDF5_STATUS(H5Aclose(dsId));
            } catch (...) {


                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }
        }


        void HashHdf5Serializer::writeSequenceAttribute(hid_t group, const std::vector< std::complex<float> >& value, const std::string & key) {
            try {
                hsize_t len = value.size();
                hsize_t dims[] = {len, 2};
                hid_t spaceId = H5Screate_simple(2, dims, NULL);
                KARABO_CHECK_HDF5_STATUS(spaceId);
                hid_t stid = karabo::io::h5::ScalarTypes::getHdf5StandardType<float>();
                hid_t ntid = karabo::io::h5::ScalarTypes::getHdf5NativeType<float>();
                hid_t dsId = H5Acreate2(group, key.c_str(), stid, spaceId, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Awrite(dsId, ntid, &value[0]));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(ntid));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(spaceId));
                KARABO_CHECK_HDF5_STATUS(H5Aclose(dsId));
            } catch (...) {


                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }

        }


        void HashHdf5Serializer::writeSequenceAttribute(hid_t group, const std::vector< std::complex<double> >& value, const std::string & key) {
            try {
                hsize_t len = value.size();
                hsize_t dims[] = {len, 2};
                hid_t spaceId = H5Screate_simple(2, dims, NULL);
                KARABO_CHECK_HDF5_STATUS(spaceId);
                hid_t stid = karabo::io::h5::ScalarTypes::getHdf5StandardType<double>();
                hid_t ntid = karabo::io::h5::ScalarTypes::getHdf5NativeType<double>();
                hid_t dsId = H5Acreate2(group, key.c_str(), stid, spaceId, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Awrite(dsId, ntid, &value[0]));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(ntid));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(spaceId));
                KARABO_CHECK_HDF5_STATUS(H5Aclose(dsId));
            } catch (...) {


                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }

        }


        void HashHdf5Serializer::writeSequenceAttribute(hid_t group, const std::vector<bool>& value, const std::string & key) {
            try {
                hsize_t len = value.size();
                hsize_t dims[] = {len};
                hid_t spaceId = H5Screate_simple(1, dims, NULL);
                KARABO_CHECK_HDF5_STATUS(spaceId);
                hid_t stid = karabo::io::h5::ScalarTypes::getHdf5StandardType<bool>();
                hid_t ntid = karabo::io::h5::ScalarTypes::getHdf5NativeType<bool>();
                std::vector<unsigned char> converted(len, 0);
                for (size_t i = 0; i < len; ++i) {
                    converted[i] = boost::numeric_cast<unsigned char>(value[i]);
                }
                const unsigned char* ptr = &converted[0];
                hid_t dsId = H5Acreate2(group, key.c_str(), stid, spaceId, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Awrite(dsId, ntid, ptr));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(ntid));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(spaceId));
                KARABO_CHECK_HDF5_STATUS(H5Aclose(dsId));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }

        }


        void HashHdf5Serializer::serializeHash(hid_t group, karabo::util::Hash& data) {
            try {
                H5G_info_t ginfo;
                KARABO_CHECK_HDF5_STATUS(H5Gget_info(group, &ginfo));
                for (hsize_t i = 0; i < ginfo.nlinks; ++i) {
                    ssize_t len = H5Lget_name_by_idx(group, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, i, NULL, 0, H5P_DEFAULT);
                    len++; //null terminated string                    
                    std::vector<char> bufName(len, 0);
                    ssize_t size = H5Lget_name_by_idx(group, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, i, &bufName[0], len, H5P_DEFAULT);
                    std::string name(&bufName[0], len - 1);
                    KARABO_CHECK_HDF5_STATUS(size);
                    H5O_info_t objInfo;
                    KARABO_CHECK_HDF5_STATUS(H5Oget_info_by_idx(group, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, i, &objInfo, H5P_DEFAULT));
                    switch (objInfo.type) {
                        case H5O_TYPE_GROUP:
                        {
                            hid_t gid = H5Gopen2(group, name.c_str(), H5P_DEFAULT);
                            KARABO_CHECK_HDF5_STATUS(gid);
                            int last = name.length() - 1;
                            if (name[last] == ']') {
                                serializeVectorOfHashesElement(gid, name, data, i, group);
                            } else {
                                serializeHashElement(gid, name, data);
                            }

                            KARABO_CHECK_HDF5_STATUS(H5Gclose(gid));
                        }
                            break;
                        case H5O_TYPE_DATASET:
                        {
                            hid_t dsId = H5Dopen2(group, name.c_str(), H5P_DEFAULT);
                            KARABO_CHECK_HDF5_STATUS(dsId);
                            serializeDataElement(dsId, name, data);
                            KARABO_CHECK_HDF5_STATUS(H5Dclose(dsId));
                        }
                            break;
                        default:
                            throw KARABO_HDF_IO_EXCEPTION("H5O_TYPE Not supported");
                    }
                }
            } catch (karabo::util::Exception& ex) {
                std::clog << ex << std::endl;
                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot serialize Hash"));
            }

        }


        void HashHdf5Serializer::serializeHashElement(hid_t group, const std::string& name, karabo::util::Hash& data) {

            try {
                karabo::util::Hash& newData = data.bindReference< karabo::util::Hash>(name);

                H5G_info_t ginfo;
                KARABO_CHECK_HDF5_STATUS(H5Gget_info(group, &ginfo));

                //                std::clog << "nobj=" << ginfo.nlinks << std::endl;
                for (hsize_t i = 0; i < ginfo.nlinks; ++i) {
                    H5O_info_t objInfo;
                    KARABO_CHECK_HDF5_STATUS(H5Oget_info_by_idx(group, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, i, &objInfo, H5P_DEFAULT));
                    ssize_t len = H5Lget_name_by_idx(group, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, i, NULL, 0, H5P_DEFAULT);
                    len++; //null terminated string                    
                    std::vector<char> bufName(len, 0);
                    ssize_t size = H5Lget_name_by_idx(group, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, i, &bufName[0], len, H5P_DEFAULT);
                    std::string name(&bufName[0], len - 1);
                    //                        std::clog << "objname: len=" << len << " name=" << name << std::endl;
                    KARABO_CHECK_HDF5_STATUS(size);
                    switch (objInfo.type) {
                        case H5O_TYPE_GROUP:
                        {
                            hid_t gid = H5Gopen2(group, name.c_str(), H5P_DEFAULT);
                            KARABO_CHECK_HDF5_STATUS(gid);
                            //                                std::clog << "group name: " << name << std::endl;
                            int last = name.length() - 1;
                            //                                std::clog << "last character of group name: " << name[last] << std::endl;
                            if (name[last] == ']') {
                                serializeVectorOfHashesElement(gid, name, newData, i, group);
                            } else {
                                serializeHashElement(gid, name, newData);
                                serializeAttributes(gid, newData.getNode(name));
                            }


                            KARABO_CHECK_HDF5_STATUS(H5Gclose(gid));
                        }
                            break;
                        case H5O_TYPE_DATASET:
                        {
                            //                            std::clog << "dataset: " << name << std::endl;
                            hid_t dsId = H5Dopen2(group, name.c_str(), H5P_DEFAULT);
                            serializeDataElement(dsId, name, newData);
                            KARABO_CHECK_HDF5_STATUS(H5Dclose(dsId));

                        }
                            break;
                        default:
                            std::clog << "unknown: " << name << std::endl;
                    }
                }
            } catch (karabo::util::Exception& ex) {
                std::clog << ex << std::endl;
                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot serialize Hash"));
            }

        }


        void HashHdf5Serializer::serializeVectorOfHashesElement(hid_t gid, const std::string& name, karabo::util::Hash& data, hsize_t& idx, hid_t group) {

            //                std::clog << "vec hashes" << std::endl;
            karabo::util::Hash a("a", 0);
            karabo::util::Hash::Node& aNode = a.getNode("a");
            serializeAttributes(gid, aNode, true);

            std::string vecHashKey = name;
            karabo::util::getAndCropIndex(vecHashKey);
            //                std::clog << "vecHashKey: " << vecHashKey << std::endl;
            unsigned long long vsize = aNode.getAttribute<unsigned long long>("KRB_size");
            //                std::clog << "Hash a: " << a << std::endl;
            //                std::clog << "vsize: " << vsize << std::endl;
            std::vector<karabo::util::Hash>& vec = data.bindReference<std::vector<karabo::util::Hash> >(vecHashKey);
            vec.resize(vsize, karabo::util::Hash());

            for (hsize_t j = 0; j < vsize; ++j, ++idx) {
                try {
                    std::ostringstream os;
                    os << vecHashKey << "[" << j << "]";
                    //                        std::clog << "vec hash name: " << os.str() << std::endl;
                    hid_t vecElementGroupId = H5Gopen2(group, os.str().c_str(), H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(vecElementGroupId);
                    serializeHash(vecElementGroupId, vec[j]);
                    KARABO_CHECK_HDF5_STATUS(H5Gclose(vecElementGroupId));
                } catch (karabo::util::Exception& ex) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot serialize Vector of Hashes"));
                }
            }
        }


        void HashHdf5Serializer::serializeDataElement(hid_t dsId, const std::string& name, karabo::util::Hash& data) {

            try {
                // std::clog << "reading dataset " << name << std::endl;
                hid_t tid = H5Dget_type(dsId);
                KARABO_CHECK_HDF5_STATUS(tid);
                hid_t spaceId = H5Dget_space(dsId);
                KARABO_CHECK_HDF5_STATUS(spaceId);
                int ndims = H5Sget_simple_extent_ndims(spaceId);
                //                std::clog << " ndims=" << ndims << std::endl;
                KARABO_CHECK_HDF5_STATUS(ndims);
                std::vector<hsize_t> dims(ndims, 0);
                ndims = H5Sget_simple_extent_dims(spaceId, &dims[0], NULL);
                KARABO_CHECK_HDF5_STATUS(ndims);

                if (ndims == 0) {
                    //                        std::clog << "ndims=0 " << std::endl;
                    H5T_class_t dtClass = H5Tget_class(tid);
                    if (dtClass == H5T_OPAQUE) {
                        char* tagPtr = H5Tget_tag(tid);
                        std::string tag(tagPtr);
                        free(tagPtr);
                        //std::clog << "opaque type: " << tag << std::endl;
                        if (tag == "CHAR") {
                            readSingleValue<char>(dsId, tid, name, data);
                        } else if (tag == "VECTOR_CHAR") {
                            readSequenceBytes(dsId, tid, name, data);
                        }
                    } else if (dtClass == H5T_STRING) {
                        readSingleString(dsId, tid, name, data);
                    } else {
                        if (H5Tequal(tid, H5T_NATIVE_INT8)) {
                            readSingleValue<signed char>(dsId, tid, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_INT16)) {
                            readSingleValue<short>(dsId, tid, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_INT32)) {
                            readSingleValue<int>(dsId, tid, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_INT64)) {
                            readSingleValue<long long>(dsId, tid, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_UINT8)) {
                            // this read unsigned char and boolean types
                            // if KRB_bool attribute  exists unsigned char value is converted to bool
                            readSingleUnsignedChar(dsId, tid, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_UINT16)) {
                            readSingleValue<unsigned short>(dsId, tid, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_UINT32)) {
                            readSingleValue<unsigned int>(dsId, tid, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_UINT64)) {
                            readSingleValue<unsigned long long>(dsId, tid, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_FLOAT)) {
                            readSingleValue<float>(dsId, tid, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_DOUBLE)) {
                            readSingleValue<double>(dsId, tid, name, data);
                        } else {
                            throw KARABO_HDF_IO_EXCEPTION("Scalar type not supported");
                        }
                    }

                } else {
                    H5T_class_t dtClass = H5Tget_class(tid);
                    // vector<char> is handled with ndims=0 as this is OPAQUE datatype with H5S_SCALAR space                        
                    if (dtClass == H5T_STRING) {
                        std::clog << "string type ndims>0 " << std::endl;
                        readSequenceString(dsId, tid, dims, name, data);
                    } else {
                        //                    std::clog << "vector read: " << name << std::endl;                                               
                        if (H5Tequal(tid, H5T_NATIVE_INT8)) {
                            readSequenceValue<signed char>(dsId, tid, dims, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_INT16)) {
                            readSequenceValue<short>(dsId, tid, dims, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_INT32)) {
                            readSequenceValue<int>(dsId, tid, dims, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_INT64)) {
                            readSequenceValue<long long>(dsId, tid, dims, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_UINT8)) {
                            // this read vectors of unsigned char and boolean types
                            // if KRB_bool attribute  exists unsigned char values are converted to bool
                            readSequenceUnsignedChar(dsId, tid, dims, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_UINT16)) {
                            readSequenceValue<unsigned short>(dsId, tid, dims, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_UINT32)) {
                            readSequenceValue<unsigned int>(dsId, tid, dims, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_UINT64)) {
                            readSequenceValue<unsigned long long>(dsId, tid, dims, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_FLOAT)) {
                            readSequenceFloatingPoint<float>(dsId, tid, dims, name, data);
                        } else if (H5Tequal(tid, H5T_NATIVE_DOUBLE)) {
                            readSequenceFloatingPoint<double>(dsId, tid, dims, name, data);
                        } else {
                            throw KARABO_HDF_IO_EXCEPTION("Sequence type not supported");
                        }
                    }
                }


                karabo::util::Hash::Node& node = data.getNode(name);
                serializeAttributes(dsId, node);
            } catch (karabo::util::Exception& ex) {
                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot read data"));
            }
        }


        void HashHdf5Serializer::serializeAttributes(hid_t h5obj, karabo::util::Hash::Node& node, bool krb) {
            H5O_info_t objInfo;
            KARABO_CHECK_HDF5_STATUS(H5Oget_info(h5obj, &objInfo));
            for (hsize_t i = 0; i < objInfo.num_attrs; ++i) {

                ssize_t len = H5Aget_name_by_idx(h5obj, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, i, NULL, 0, H5P_DEFAULT);
                len++; //null terminated string                    
                std::vector<char> bufName(len, 0);
                ssize_t size = H5Aget_name_by_idx(h5obj, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, i, &bufName[0], len, H5P_DEFAULT);
                std::string name(&bufName[0]);
                if (krb == false && name.substr(0, 4) == "KRB_") continue;
                //                    std::clog << "attributes objname: len=" << len << " name=" << name << std::endl;
                KARABO_CHECK_HDF5_STATUS(size);
                hid_t attrId = H5Aopen_by_idx(h5obj, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, i, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(attrId);

                hid_t tid = H5Aget_type(attrId);
                KARABO_CHECK_HDF5_STATUS(tid);

                hid_t spaceId = H5Aget_space(attrId);
                KARABO_CHECK_HDF5_STATUS(spaceId);

                int ndims = H5Sget_simple_extent_ndims(spaceId);
                //                std::clog << " ndims=" << ndims << std::endl;
                KARABO_CHECK_HDF5_STATUS(ndims);
                std::vector<hsize_t> dims(ndims, 0);
                ndims = H5Sget_simple_extent_dims(spaceId, &dims[0], NULL);
                KARABO_CHECK_HDF5_STATUS(ndims);

                if (ndims == 0) {
                    if (H5Tequal(tid, H5T_NATIVE_CHAR)) {
                        readSingleAttribute<char>(attrId, tid, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_INT8)) {
                        readSingleAttribute<signed char>(attrId, tid, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_INT16)) {
                        readSingleAttribute<short>(attrId, tid, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_INT32)) {
                        readSingleAttribute<int>(attrId, tid, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_INT64)) {
                        readSingleAttribute<long long>(attrId, tid, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_UINT8)) {
                        readSingleAttribute<unsigned char>(attrId, tid, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_UINT16)) {
                        readSingleAttribute<unsigned short>(attrId, tid, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_UINT32)) {
                        readSingleAttribute<unsigned int>(attrId, tid, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_UINT64)) {
                        readSingleAttribute<unsigned long long>(attrId, tid, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_FLOAT)) {
                        readSingleAttribute<float>(attrId, tid, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_DOUBLE)) {
                        readSingleAttribute<double>(attrId, tid, node, name);
                    } else {
                        std::clog << "Type not supported for attribute: " << name << std::endl;
                    }

                } else {
                    if (H5Tequal(tid, H5T_NATIVE_CHAR)) {
                        readSequenceAttribute<char>(attrId, tid, dims, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_INT8)) {
                        readSequenceAttribute<signed char>(attrId, tid, dims, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_INT16)) {
                        readSequenceAttribute<short>(attrId, tid, dims, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_INT32)) {
                        readSequenceAttribute<int>(attrId, tid, dims, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_INT64)) {
                        readSequenceAttribute<long long>(attrId, tid, dims, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_UINT8)) {
                        readSequenceAttribute<unsigned char>(attrId, tid, dims, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_UINT16)) {
                        readSequenceAttribute<unsigned short>(attrId, tid, dims, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_UINT32)) {
                        readSequenceAttribute<unsigned int>(attrId, tid, dims, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_UINT64)) {
                        readSequenceAttribute<unsigned long long>(attrId, tid, dims, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_FLOAT)) {
                        readSequenceAttribute<float>(attrId, tid, dims, node, name);
                    } else if (H5Tequal(tid, H5T_NATIVE_DOUBLE)) {
                        readSequenceAttribute<double>(attrId, tid, dims, node, name);
                    } else {
                        std::clog << "Type not supported for attribute: " << name << std::endl;
                    }

                }
                KARABO_CHECK_HDF5_STATUS(H5Aclose(attrId));
            }

        }


        void HashHdf5Serializer::readSingleString(hid_t dsId, hid_t tid, const std::string& name, karabo::util::Hash& data) {
            size_t len = H5Tget_size(tid);
            KARABO_CHECK_HDF5_STATUS(len);
            hid_t stringTypeId = H5Tcopy(H5T_C_S1);
            KARABO_CHECK_HDF5_STATUS(H5Tset_size(stringTypeId, len));
            if (H5Tequal(tid, stringTypeId)) {
                std::vector<char> vec(len, 0);
                //std::clog << "C_S1 1" << std::endl;
                KARABO_CHECK_HDF5_STATUS(H5Dread(dsId, tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vec[0]));
                //std::clog << "C_S1 2" << std::endl;
                data.set(name, std::string(&vec[0]));
                //std::clog << "C_S1 3" << std::endl;
            } else {
                throw KARABO_HDF_IO_EXCEPTION("Type not supported");
            }
        }
        

        void HashHdf5Serializer::readSingleUnsignedChar(hid_t dsId, hid_t tid, const std::string& name, karabo::util::Hash& data) {
            unsigned char value;
            KARABO_CHECK_HDF5_STATUS(H5Dread(dsId, tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value));
            htri_t exists = H5Aexists(dsId, "KRB_bool");
            KARABO_CHECK_HDF5_STATUS(exists);
            if (exists) {
                bool bValue = boost::numeric_cast<bool>(value);
                data.set(name, bValue);
            } else {
                data.set(name, value);
            }
        }


        void HashHdf5Serializer::readSequenceString(hid_t dsId, hid_t tid, const std::vector<hsize_t>& dims, const std::string& name, karabo::util::Hash& data) {
            hsize_t size = dims[0];
            for (size_t i = 1; i < dims.size(); ++i) {
                size *= dims[i];
            }
            std::vector<std::string>& vec = data.bindReference<std::vector<std::string> >(name);
            vec.resize(size, "");
            std::vector<char*> ptr(size, 0);
            char** chPtr = &ptr[0];
            KARABO_CHECK_HDF5_STATUS(H5Dread(dsId, tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, chPtr));
            for (size_t i = 0; i < size; ++i) {
                vec[i] = std::string(ptr[i]);
            }
        }


        void HashHdf5Serializer::readSequenceUnsignedChar(hid_t dsId, hid_t tid, const std::vector<hsize_t>& dims, const std::string& name, karabo::util::Hash& data) {
            hsize_t size = dims[0];
            for (size_t i = 1; i < dims.size(); ++i) {
                size *= dims[i];
            }
            vector<unsigned char> vec(size, 0);
            KARABO_CHECK_HDF5_STATUS(H5Dread(dsId, tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vec[0]));
            htri_t exists = H5Aexists(dsId, "KRB_bool");
            KARABO_CHECK_HDF5_STATUS(exists);
            if (exists) {
                vector<bool>& bVec = data.bindReference < vector<bool> >(name);
                bVec.resize(size, false);
                for (size_t i = 0; i < size; ++i) {
                    bVec[i] = boost::numeric_cast<bool>(vec[i]);
                }
            } else {
                data.set(name, vec);
            }
        }


        void HashHdf5Serializer::readSequenceBytes(hid_t dsId, hid_t tid, const std::string& name, karabo::util::Hash& data) {
            std::vector<char>& vec = data.bindReference<std::vector<char> >(name);
            hsize_t len = H5Tget_size(tid);
            KARABO_CHECK_HDF5_STATUS(len);
            vec.resize(len);
            char* ptr = &vec[0];
            KARABO_CHECK_HDF5_STATUS(H5Dread(dsId, tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, ptr));
        }



    }
}

