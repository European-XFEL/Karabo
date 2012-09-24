/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 11, 2012, 10:03 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_XIP_HASHBINARYSERIALIZER_HH
#define	EXFEL_XIP_HASHBINARYSERIALIZER_HH

#include "Format.hh" // Remove later

#include "BinarySerializer.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

    namespace io {

        /**
         * The HashBinarySerializer class.
         */
        class HashBinarySerializer : public BinarySerializer<exfel::util::Hash> {
            
        public:
            
            EXFEL_CLASSINFO(HashBinarySerializer, "Default", "1.0")
            
            /**
             * Default constructor.
             */
            HashBinarySerializer();
            
            
            static void expectedParameters(exfel::util::Schema& expected) {};

            void configure(const exfel::util::Hash& input) {};
            
             virtual void save(const exfel::util::Hash& object, std::vector<char>& archive) {
                 std::string tmp = m_hashFormat->serialize(object);
                 archive.assign(tmp.begin(), tmp.end());
             }
             
             virtual void load(exfel::util::Hash& object, const char* archive, const size_t nBytes) {
                 std::stringstream str;
                 str.rdbuf()->pubsetbuf(const_cast<char*>(archive), nBytes);
                 m_hashFormat->convert(str, object);
             }

            /**
             * Destructor.
             */
            virtual ~HashBinarySerializer() {
            };


        private: // members
            
            // This is a quick hack for now
            // TODO Re-implement
            Format<exfel::util::Hash>::Pointer m_hashFormat;


        private: // functions

        };

    }
}

#endif
