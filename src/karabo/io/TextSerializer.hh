/*
 * $Id$
 *
 * File:   TextSerializer.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 20, 2013, 11:05 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_IO_TEXTSERIALIZER_HH
#define	KARABO_IO_TEXTSERIALIZER_HH

#include <vector>
#include <sstream>
#include <karabo/util/Configurator.hh>

namespace karabo {
    namespace io {

        template <class T>
        class TextSerializer {

            public:

            KARABO_CLASSINFO(TextSerializer<T>, "TextSerializer", "1.0")

            KARABO_CONFIGURATION_BASE_CLASS

            virtual void save(const T& object, std::string& archive) = 0;

            virtual void save(const std::vector<T>& objects, std::string& archive) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Saving vectors of objects is not supported by this serializer");
            }

            virtual void load(T& object, const std::string& archive) = 0;

            virtual void load(T& object, const std::stringstream& archive) {
                this->load(object, archive.str()); // Creates a copy, but may be overridden for more performance
            }

            virtual void load(T& object, const char* archive) {
                this->load(object, std::string(archive)); // Creates a copy, but may be overridden for more performance
            }

            virtual void load(T& object, const char* archive, const size_t nBytes) {
                this->load(object, std::string(archive, nBytes));
            }

            virtual void load(std::vector<T>& objects, const std::string& archive) {
                std::vector<T> tmp(1);
                this->load(tmp[0], archive);
                objects.swap(tmp);
            }

            virtual void load(std::vector<T>& objects, const std::stringstream& archive) {
                this->load(objects, archive.str());
            }

            virtual std::string save(const T& object) {
                std::string archive;
                this->save(object, archive);
                return archive;
            }

            virtual T load(const std::string& archive) {
                T object;
                this->load(object, archive);
                return object;
            }

            virtual T load(const char* archive) {
                T object;
                this->load(object, archive);
                return object;
            }

            virtual T load(char* archive, const size_t nBytes) {
                T object;
                this->load(object, archive, nBytes);
                return object;
            }

        };
    }
}

#endif
