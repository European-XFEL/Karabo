/* 
 * File:   SchemaBinarySerializer.hh
 * Author: <djelloul.boukhelef@xfel.eu>
 *
 * Created on July 9, 2013, 8:58 AM
 */

#ifndef SCHEMABINARYSERIALIZER_HH
#define	SCHEMABINARYSERIALIZER_HH

#include <karabo/util/Schema.hh>

#include "BinarySerializer.hh"

namespace karabo {

    namespace io {

        class SchemaBinarySerializer : public BinarySerializer<karabo::util::Schema> {


            BinarySerializer<karabo::util::Hash>::Pointer m_serializer;

        public:

            KARABO_CLASSINFO(SchemaBinarySerializer, "Bin", "1.0")

            SchemaBinarySerializer(const karabo::util::Hash& hash);
            virtual ~SchemaBinarySerializer();

            static void expectedParameters(karabo::util::Schema& expected);

            void save(const karabo::util::Schema& object, std::vector<char>& archive);
            void load(karabo::util::Schema& object, const char* archive, const size_t nBytes);
        };
    }
}

KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::BinarySerializer<karabo::util::Schema>)

#endif	/* SCHEMABINARYSERIALIZER_HH */

