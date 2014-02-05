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

        template <class T>
        class BinarySerializer {

        public:

            KARABO_CLASSINFO(BinarySerializer, "BinarySerializer", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS;

            virtual void save(const T& object, std::vector<char>& archive) = 0;

            virtual void load(T& object, const char* archive, const size_t nBytes) = 0;

            void load(T& object, const std::vector<char>& archive) {
                load(object, &archive[0], archive.size());
            }

            std::vector<char> save(const T& object) {
                std::vector<char> archive;
                this->save(object, archive);
                return archive;
            }

            T load(const char* archive, const size_t nBytes) {
                T object;
                this->load(object, archive, nBytes);
                return object;
            }

            T load(const std::vector<char>& archive) {
                T object;
                this->load(object, archive);
                return object;
            }

            virtual void save(const std::vector<T>& objects, std::vector<char>& archive) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Saving vectors of objects is not supported by this serializer");
            }

            virtual void load(std::vector<T>& objects, const char* archive, const size_t nBytes) {
                std::vector<T> tmp(1);
                this->load(tmp[0], archive, nBytes);
                objects.swap(tmp);
            }

            void load(std::vector<T>& objects, const std::vector<char>& archive) {
                load(objects, &archive[0], archive.size());
            }

        };
    }
}

namespace karabo { namespace util { class Hash; } }
KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::BinarySerializer<karabo::util::Hash>)

namespace karabo { namespace util { class Schema; } }
KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::BinarySerializer<karabo::util::Schema>)

namespace karabo { namespace xip { template <class T> class CpuImage; } }
KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::BinarySerializer<karabo::xip::CpuImage<float> >)
KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::BinarySerializer<karabo::xip::CpuImage<double> >)

#endif
