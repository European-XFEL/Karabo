/*
 * $Id$
 *
 * File:   TextSerializer.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 20, 2013, 11:05 AM
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

#ifndef KARABO_DATA_IO_TEXTSERIALIZER_HH
#define KARABO_DATA_IO_TEXTSERIALIZER_HH

#include <sstream>
#include <vector>

#include "karabo/data/schema/Configurator.hh"

namespace karabo {
    namespace data {

        /*
         * @class TextSerializer
         * @brief The TextSerializer implements a text format serialization and
         *        de-serialization interface for type T. The actual serialization
         *        is logic is implemented in derived class.
         */
        template <class T>
        class TextSerializer {
           public:
            KARABO_CLASSINFO(TextSerializer<T>, "TextSerializer", "1.0")

            KARABO_CONFIGURATION_BASE_CLASS

            /**
             * Save an object into a text archive
             * @param object to save
             * @param archive to save into
             */
            virtual void save(const T& object, std::string& archive) = 0;

            /**
             * Save a vector of objects into a text archive
             * @param objects to save
             * @param archive to save into
             */
            virtual void save(const std::vector<T>& objects, std::string& archive) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Saving vectors of objects is not supported by this serializer");
            }

            /**
             * Load an object from a text archive
             * @param object to load data into
             * @param archive to load the object from
             */
            virtual void load(T& object, const std::string& archive) = 0;

            /**
             * Load an object from a text archive
             * @param object to load data into
             * @param archive to load the object from
             */
            virtual void load(T& object, const std::stringstream& archive) {
                this->load(object, archive.str()); // Creates a copy, but may be overridden for more performance
            }
            /**
             * Load an object from a text archive
             * @param object to load data into
             * @param archive to load the object from
             */
            virtual void load(T& object, const char* archive) {
                this->load(object,
                           std::string(archive ? archive
                                               : "")); // Creates a copy, but may be overridden for more performance
            }

            /**
             * Load an object from a text archive
             * @param object to load data into
             * @param archive to load the object from
             * @param nBytes size of the archive
             */
            virtual void load(T& object, const char* archive, const size_t nBytes) {
                this->load(object, std::string(archive, nBytes));
            }

            /**
             * Save a vector of objects into a text archive
             * @param objects to save
             * @param archive to save into
             */
            virtual void load(std::vector<T>& objects, const std::string& archive) {
                std::vector<T> tmp(1);
                this->load(tmp[0], archive);
                objects.swap(tmp);
            }

            /**
             * Save a vector of objects into a text archive
             * @param objects to save
             * @param archive to save into
             */
            virtual void load(std::vector<T>& objects, const std::stringstream& archive) {
                this->load(objects, archive.str());
            }

            /**
             * Return the serialized text representation of an object, i.e. save into an empty
             * archive and return this
             * @param object
             * @return
             */
            virtual std::string save(const T& object) {
                std::string archive;
                this->save(object, archive);
                return archive;
            }

            /**
             * Load an object from a text archive.
             * @param archive binary text containing the data
             */
            virtual T load(const std::string& archive) {
                T object;
                this->load(object, archive);
                return object;
            }

            /**
             * Load an object from a text archive.
             * @param archive binary text containing the data
             */
            virtual T load(const char* archive) {
                T object;
                this->load(object, archive);
                return object;
            }

            /**
             * Load an object from a text archive.
             * @param archive binary text containing the data
             * @param nBytes size in bytes of the data archive
             */
            virtual T load(char* archive, const size_t nBytes) {
                T object;
                this->load(object, archive, nBytes);
                return object;
            }
        };
    } // namespace data
} // namespace karabo

#endif
