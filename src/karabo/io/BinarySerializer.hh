/*
 * $Id$
 *
 * File:   BinarySerializer.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 10, 2012, 5:05 PM
 *
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

#ifndef KARABO_IO_BINARYSERIALIZER_HH
#define KARABO_IO_BINARYSERIALIZER_HH

#include <vector>

#include "karabo/data/schema/Configurator.hh"

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
             * @param archive to save to - some implementations clear() it before appending (e.g. in
             *                HashBinarySerializer), some do not (e.g. in SchemaBinarySerializer)
             */
            virtual void save(const T& object, std::vector<char>& archive) = 0;

            /**
             * Save an object by appending it to a binary archive
             * @param object to save
             * @param archive to append to - no clear() called
             */
            virtual void save2(const T& object, std::vector<char>& archive) = 0;

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
             * @return number of processed bytes in archive
             */
            virtual size_t load(T& object, const char* archive, const size_t nBytes) = 0;

            /**
             * Load an object from a binary archive.
             * @param object to load data into
             * @param archive binary archive containing the data
             * @return number of processed bytes in archive
             */
            size_t load(T& object, const std::vector<char>& archive) {
                if (archive.empty()) return 0;
                else return load(object, &archive[0], archive.size());
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
             * @brief Loads the last object from a sequence of concatenated binary serialized objects of
             * the same type. Sequences with only one object are also supported.
             *
             * @param object to load the last instance into.
             * @param archive sequence containing 1 or more objects of type T in binary serialized form.
             */
            void loadLastFromSequence(T& object, const char* archive, const std::size_t nBytes) {
                const std::size_t fullSize = nBytes;
                std::size_t readSize = 0u;
                while (readSize < fullSize) {
                    readSize += this->load(object, archive + readSize, fullSize - readSize);
                };
            }

            /**
             * Save an object to a binary archive
             * @param object to save
             * @param archive to save to
             */
            void save(const std::shared_ptr<T>& object, std::vector<char>& archive) {
                save(*object, archive);
            }

            /**
             * Load an object from a binary archive.
             * @param object to load data into
             * @param archive binary archive containing the data
             * @param nBytes size in bytes of the data archive
             * @return number of processed bytes in archive
             */
            size_t load(const std::shared_ptr<T>& object, const char* archive, const size_t nBytes) {
                return load(*object, archive, nBytes);
            }

            /**
             * Load an object from a binary archive.
             * @param object to load data into
             * @param archive binary archive containing the data
             * @return number of processed bytes in archive
             */
            size_t load(const std::shared_ptr<T>& object, const std::vector<char>& archive) {
                if (archive.empty()) return 0;
                else return load(*object, &archive[0], archive.size());
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
             * @return number of processed bytes in archive
             */
            virtual size_t load(std::vector<T>& objects, const char* archive, const size_t nBytes) {
                std::vector<T> tmp(1);
                size_t bytes = this->load(tmp[0], archive, nBytes);
                objects.swap(tmp);
                return bytes;
            }

            /**
             * Load a vector of objects from a binary archive.
             * @param objects to load data into
             * @param archive binary archive containing the data
             */
            size_t load(std::vector<T>& objects, const std::vector<char>& archive) {
                if (archive.empty()) return 0;
                else return load(objects, &archive[0], archive.size());
            }
        };
    } // namespace io
} // namespace karabo

#endif
