/*
 * $Id: Format.hh 9794 2013-05-23 12:41:42Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */





#ifndef KARABO_IO_H5_HASHHDF5SERIALIZER_HH
#define	KARABO_IO_H5_HASHHDF5SERIALIZER_HH

#include "h5/ErrorHandler.hh"


#include <iostream>
#include <hdf5/hdf5.h>
#include "Hdf5Serializer.hh"
#include <karabo/io/h5/ErrorHandler.hh>
#include <karabo/io/h5/TypeTraits.hh>
#include <karabo/io/h5/VLArray.hh>


#include <string>
#include <vector>



namespace karabo {
    namespace io {

        class HashHdf5Serializer : public Hdf5Serializer {

        public:

            KARABO_CLASSINFO(HashHdf5Serializer, "h5", "1.0")


            HashHdf5Serializer(const karabo::util::Hash& input) : Hdf5Serializer(input) {
                m_stringStid = karabo::io::h5::ScalarTypes::getHdf5StandardType<std::string>();
                m_stringNtid = karabo::io::h5::ScalarTypes::getHdf5NativeType<std::string>();
                m_spaceId = H5Screate(H5S_SCALAR);

                m_gcpl = H5Pcreate(H5P_GROUP_CREATE);
                KARABO_CHECK_HDF5_STATUS(m_gcpl);
                KARABO_CHECK_HDF5_STATUS(H5Pset_link_creation_order(m_gcpl, H5P_CRT_ORDER_TRACKED | H5P_CRT_ORDER_INDEXED));

            };

            virtual ~HashHdf5Serializer() {
                KARABO_CHECK_HDF5_STATUS(H5Pclose(m_gcpl));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(m_spaceId));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(m_stringNtid));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(m_stringStid));

            }

        private:

            virtual void save(const karabo::util::Hash& object, hid_t h5file, const std::string& groupName) {
                hid_t group = H5Gcreate(h5file, groupName.c_str(), H5P_DEFAULT, m_gcpl, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(group);
                serializeHash(object, group);
                KARABO_CHECK_HDF5_STATUS(H5Gclose(group));
            }
        private:

            // members


            hid_t m_spaceId;
            hid_t m_stringStid;
            hid_t m_stringNtid;
            hid_t m_gcpl;

            // functions

            void serializeHash(const karabo::util::Hash& data, hid_t group);

            void serializeHashElement(const karabo::util::Hash::Node& el, hid_t group);

            void serializeVectorOfHashesElement(const karabo::util::Hash::Node& el, hid_t h5obj);

            void serializeDataElement(const karabo::util::Hash::Node& el, hid_t h5obj);

            void serializeAttributes(const karabo::util::Hash::Node& el, hid_t h5obj);

            template<class T>
            void serializeNode(const karabo::util::Hash::Node& node, hid_t group);

            void serializeNodeByte(const karabo::util::Hash::Node& node, hid_t group);

            void serializeNodeString(const karabo::util::Hash::Node& node, hid_t group);

            void serializeNodeBool(const karabo::util::Hash::Node& node, hid_t group);

            template<class U>
            void serializeNodeComplex(const karabo::util::Hash::Node& node, hid_t group);

            template<typename T>
            void serializeNodeSequence(const karabo::util::Hash::Node& node, hid_t group);

            void serializeNodeSequenceBool(const karabo::util::Hash::Node& node, hid_t group);

            template<class U>
            void serializeNodeSequenceComplex(const karabo::util::Hash::Node& node, hid_t group);

            void serializeNodeSequenceByte(const karabo::util::Hash::Node& node, hid_t group);

            // Attributes

            template<typename T>
            void writeSingleAttribute(hid_t group, const T& value, const std::string& key);

            void writeSingleAttribute(hid_t group, const std::string& value, const std::string& key);

            void writeSingleAttribute(hid_t group, const bool& value, const std::string& key);

            void writeSingleAttribute(hid_t group, const std::complex<float>& value, const std::string& key);

            void writeSingleAttribute(hid_t group, const std::complex<double>& value, const std::string& key);

            template<typename T>
            void writeSequenceAttribute(hid_t group, const std::vector<T>& value, const std::string& key);

            void writeSequenceAttribute(hid_t group, const std::vector< std::complex<float> >& value, const std::string& key);

            void writeSequenceAttribute(hid_t group, const std::vector< std::complex<double> >& value, const std::string& key);

            void writeSequenceAttribute(hid_t group, const std::vector<bool>& value, const std::string& key);



        public:

            void load(karabo::util::Hash& object, hid_t h5file, const std::string& groupName) {

                hid_t group = H5Gopen2(h5file, groupName.c_str(), H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(group);
                serializeHash(group, object);
                KARABO_CHECK_HDF5_STATUS(H5Gclose(group));



            }

            void load(karabo::util::Hash& object, const std::string& filename) {
                //                std::clog << "load 1" << std::endl;
                hid_t h5file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(h5file);
                hid_t group = H5Gopen2(h5file, "0", H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(group);
                serializeHash(group, object);
                KARABO_CHECK_HDF5_STATUS(H5Gclose(group));
                KARABO_CHECK_HDF5_STATUS(H5Fclose(h5file));
            }

        private:

            void serializeHash(hid_t group, karabo::util::Hash& data);

            void serializeHashElement(hid_t group, const std::string& name, karabo::util::Hash& data);

            void serializeVectorOfHashesElement(hid_t gid, const std::string& name, karabo::util::Hash& data, hsize_t& idx, hid_t group);

            void serializeDataElement(hid_t dsId, const std::string& name, karabo::util::Hash& data);

            void serializeAttributes(hid_t h5obj, karabo::util::Hash::Node& node);

            template<class T>
            void readSingleAttribute(hid_t attrId, hid_t typeId, karabo::util::Hash::Node& node, const std::string& name) {
                T value;
                KARABO_CHECK_HDF5_STATUS(H5Aread(attrId, typeId, &value));
                node.setAttribute(name, value);
            }

            template<class T>
            void readSequenceAttribute(hid_t attrId, hid_t typeId, const std::vector<hsize_t>& dims, karabo::util::Hash::Node& node, const std::string& name) {
                hsize_t size = dims[0];
                for (size_t i = 1; i < dims.size(); ++i) {
                    size *= dims[i];
                }
                node.setAttribute(name, std::vector<T>(0, 0));
                std::vector<T>& vec = node.getAttribute<std::vector<T> >(name);
                vec.resize(size, 0);
                KARABO_CHECK_HDF5_STATUS(H5Aread(attrId, typeId, &vec[0]));
            }
           
            template<class T>
            void readSingleValue(hid_t dsId, hid_t tid, const std::string& name, karabo::util::Hash& data) {
                T& value = data.bindReference<T>(name);
                KARABO_CHECK_HDF5_STATUS(H5Dread(dsId, tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value));
            }

            void readSingleString(hid_t dsId, hid_t tid, const std::string& name, karabo::util::Hash& data);

            void readSingleBool(hid_t dsId, hid_t tid, const std::string& name, karabo::util::Hash& data);
                        
            template<class T>
            void readSequenceValue(hid_t dsId, hid_t tid, const std::vector<hsize_t>& dims, const std::string& name, karabo::util::Hash& data) {
                hsize_t size = dims[0];
                for (size_t i = 1; i < dims.size(); ++i) {
                    size *= dims[i];
                }
                std::vector<T>& vec = data.bindReference<std::vector<T> >(name);
                vec.resize(size, 0);
                KARABO_CHECK_HDF5_STATUS(H5Dread(dsId, tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vec[0]));
            }

            void readSequenceString(hid_t dsId, hid_t tid, const std::vector<hsize_t>& dims, const std::string& name, karabo::util::Hash& data);

            void readSequenceBytes(hid_t dsId, hid_t tid, const std::string& name, karabo::util::Hash& data);

        };

        template<class T>
        inline void HashHdf5Serializer::serializeNode(const karabo::util::Hash::Node& node, hid_t group) {

            const std::string& key = node.getKey();
            try {
                const T& value = node.getValue<T>();
                hid_t stid = karabo::io::h5::ScalarTypes::getHdf5StandardType<T>();
                hid_t ntid = karabo::io::h5::ScalarTypes::getHdf5NativeType<T>();
                hid_t dsId = H5Dcreate2(group, key.c_str(), stid, m_spaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dwrite(dsId, ntid, m_spaceId, m_spaceId, H5P_DEFAULT, &value));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(ntid));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stid));
                serializeAttributes(node, dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dclose(dsId));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }
        }

        template<class U>
        inline void HashHdf5Serializer::serializeNodeComplex(const karabo::util::Hash::Node& node, hid_t group) {
            const std::string& key = node.getKey();
            try {
                const std::complex<U>& value = node.getValue<std::complex<U> >();
                hid_t stid = karabo::io::h5::ScalarTypes::getHdf5StandardType<float>();
                hid_t ntid = karabo::io::h5::ScalarTypes::getHdf5NativeType<float>();
                hsize_t dims[] = {2};
                hid_t spaceId = H5Screate_simple(1, dims, NULL);
                hid_t dsId = H5Dcreate2(group, key.c_str(), stid, spaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dwrite(dsId, ntid, spaceId, spaceId, H5P_DEFAULT, &value));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(ntid));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(spaceId));
                serializeAttributes(node, dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dclose(dsId));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }
        }

        template<typename T>
        inline void HashHdf5Serializer::serializeNodeSequence(const karabo::util::Hash::Node& node, hid_t group) {
            const std::string& key = node.getKey();
            try {
                const std::vector<T>& value = node.getValue<std::vector<T> >();
                hsize_t len = value.size();
                hsize_t dims[] = {len};
                hid_t spaceId = H5Screate_simple(1, dims, NULL);
                KARABO_CHECK_HDF5_STATUS(spaceId);
                hid_t stid = karabo::io::h5::ScalarTypes::getHdf5StandardType<T>();
                hid_t ntid = karabo::io::h5::ScalarTypes::getHdf5NativeType<T>();
                hid_t dsId = H5Dcreate2(group, key.c_str(), stid, spaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dwrite(dsId, ntid, spaceId, spaceId, H5P_DEFAULT, &value[0]));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(ntid));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(spaceId));
                serializeAttributes(node, dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dclose(dsId));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }
        }

        template<class U>
        inline void HashHdf5Serializer::serializeNodeSequenceComplex(const karabo::util::Hash::Node& node, hid_t group) {
            const std::string& key = node.getKey();
            try {
                const std::vector<std::complex<U> >& value = node.getValue<std::vector< std::complex<U> > >();
                hsize_t len = value.size();
                hsize_t dims[] = {len, 2};
                hid_t spaceId = H5Screate_simple(2, dims, NULL);
                KARABO_CHECK_HDF5_STATUS(spaceId);
                hid_t stid = karabo::io::h5::ScalarTypes::getHdf5StandardType<float>();
                hid_t ntid = karabo::io::h5::ScalarTypes::getHdf5NativeType<float>();
                hid_t dsId = H5Dcreate2(group, key.c_str(), stid, spaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dwrite(dsId, ntid, spaceId, spaceId, H5P_DEFAULT, &value[0]));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(ntid));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(spaceId));
                serializeAttributes(node, dsId);
                KARABO_CHECK_HDF5_STATUS(H5Dclose(dsId));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }

        }


        // Attributes

        template<typename T>
        inline void HashHdf5Serializer::writeSingleAttribute(hid_t group, const T& value, const std::string& key) {
            try {
                hid_t stid = karabo::io::h5::ScalarTypes::getHdf5StandardType<T>();
                hid_t ntid = karabo::io::h5::ScalarTypes::getHdf5NativeType<T>();
                hid_t dsId = H5Acreate2(group, key.c_str(), stid, m_spaceId, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(dsId);
                KARABO_CHECK_HDF5_STATUS(H5Awrite(dsId, ntid, &value));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(ntid));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stid));
                KARABO_CHECK_HDF5_STATUS(H5Aclose(dsId));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + key));
            }
        }

        template<typename T>
        inline void HashHdf5Serializer::writeSequenceAttribute(hid_t group, const std::vector<T>& value, const std::string& key) {
            try {
                hsize_t len = value.size();
                hsize_t dims[] = {len};
                hid_t spaceId = H5Screate_simple(1, dims, NULL);
                KARABO_CHECK_HDF5_STATUS(spaceId);
                hid_t stid = karabo::io::h5::ScalarTypes::getHdf5StandardType<T>();
                hid_t ntid = karabo::io::h5::ScalarTypes::getHdf5NativeType<T>();
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



    }
}


#endif

