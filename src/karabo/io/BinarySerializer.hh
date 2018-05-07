/*
 * $Id$
 *
 * File:   BinarySerializer.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 10, 2012, 5:05 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_IO_BINARYSERIALIZER_HH
#define	KARABO_IO_BINARYSERIALIZER_HH

#include <vector>

#include <karabo/util/Configurator.hh>

namespace karabo {
    namespace io {
        
        class BufferSet;

        /*
         * @class BinarySerializer
         * @brief The BinarySerializer implements a binary format serialization and
         *        de-serialization interface for type T. The actual serialization
         *        is logic is implemented in derived class.
         */
        template <class T>
        class BinarySerializer {

            public:

            KARABO_CLASSINFO(BinarySerializer, "BinarySerializer", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS;

            /**
             * Save an object to a binary archive
             * @param object to save
             * @param archive to save to
             */
            virtual void save(const T& object, std::vector<char>& archive) = 0;

            /**
             * Save an object to the BufferSet
             * @param object
             * @param archive
             */
            virtual void save(const T& object, karabo::io::BufferSet& archive) {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("save exception");
            }
            
            /**
             * Load an object from a binary archive.
             * @param object to load data into
             * @param archive binary archive containing the data
             * @param nBytes size in bytes of the data archive
             */
            virtual void load(T& object, const char* archive, const size_t nBytes) = 0;

            /**
             * Load an object from a binary archive.
             * @param object to load data into
             * @param archive binary archive containing the data
             */
            void load(T& object, const std::vector<char>& archive) {
                load(object, &archive[0], archive.size());
            }

            /**
             * Load an object from BufferSet archive
             * @param object
             * @param archive BufferSet object
             */
            virtual void load(T& object, const karabo::io::BufferSet& archive) {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("load exception");
            }

             /**
             * Save an object to a binary archive
             * @param object to save
             * @param archive to save to
             */
            void save(const boost::shared_ptr<T>& object, std::vector<char>& archive) {
                save(*object, archive);
            }

            /**
             * Load an object from a binary archive.
             * @param object to load data into
             * @param archive binary archive containing the data
             * @param nBytes size in bytes of the data archive
             */
            void load(const boost::shared_ptr<T>& object, const char* archive, const size_t nBytes) {
                load(*object, archive, nBytes);
            }

            /**
             * Load an object from a binary archive.
             * @param object to load data into
             * @param archive binary archive containing the data
             */
            void load(const boost::shared_ptr<T>& object, const std::vector<char>& archive) {
                load(*object, &archive[0], archive.size());
            }

            /**
             * Return the serialized binary representation of an object, i.e. save into an empty
             * archive and return this
             * @param object
             * @return 
             */
            std::vector<char> save(const T& object) {
                std::vector<char> archive;
                this->save(object, archive);
                return archive;
            }

            /**
             * Load an object from a binary archive.
             * @param archive binary archive containing the data
             * @param nBytes size in bytes of the data archive
             */
            T load(const char* archive, const size_t nBytes) {
                T object;
                this->load(object, archive, nBytes);
                return object;
            }

            /**
             * Load an object from a binary archive.
             * @param archive binary archive containing the data
             */
            T load(const std::vector<char>& archive) {
                T object;
                this->load(object, archive);
                return object;
            }

            /**
             * Save a vector of objects into a binary archive
             * @param objects
             * @param archive
             */
            virtual void save(const std::vector<T>& objects, std::vector<char>& archive) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Saving vectors of objects is not supported by this serializer");
            }

            /**
             * Load a vector of objects from a binary archive.
             * @param objects to load data into
             * @param archive binary archive containing the data
             * @param nBytes size in bytes of the data archive
             */
            virtual void load(std::vector<T>& objects, const char* archive, const size_t nBytes) {
                std::vector<T> tmp(1);
                this->load(tmp[0], archive, nBytes);
                objects.swap(tmp);
            }

            /**
             * Load a vector of objects from a binary archive.
             * @param objects to load data into
             * @param archive binary archive containing the data
             */
            void load(std::vector<T>& objects, const std::vector<char>& archive) {
                load(objects, &archive[0], archive.size());
            }

        };
    }
}

#endif
