/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 * 
 * Created on September 5, 2013, 5:53 PM
 */

#ifndef KARABO_XIP_RAWIMAGEDATABINARYSERIALIZER_HH
#define	KARABO_XIP_RAWIMAGEDATABINARYSERIALIZER_HH

#include <karabo/io/BinarySerializer.hh>

#include "RawImageData.hh"
/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace xip {

        class RawImageBinarySerializer : public karabo::io::BinarySerializer<RawImageData> {

            typedef karabo::io::BinarySerializer<karabo::util::Hash> HashSerializer;
            
            HashSerializer::Pointer m_hashSerializer;
            
        public:

            KARABO_CLASSINFO(RawImageBinarySerializer, "Bin", "1.0")

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
            }

            RawImageBinarySerializer(const karabo::util::Hash& input) {
                m_hashSerializer = HashSerializer::create("Bin", karabo::util::Hash(), false); // No validation for speed
            }

            void save(const RawImageData& image, std::vector<char>& archive) {
                m_hashSerializer->save(image.hash(), archive);
            }

            void load(RawImageData& image, const char* archive, const size_t nBytes) {                
                m_hashSerializer->load(image.hash(), archive, nBytes);
            }

            virtual ~RawImageBinarySerializer() {
            }
        };
    }
}


#endif

