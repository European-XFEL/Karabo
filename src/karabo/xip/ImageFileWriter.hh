/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XIP_ANYFORMATIMAGEWRITER_HH
#define	KARABO_XIP_ANYFORMATIMAGEWRITER_HH

#include <karabo/util/Factory.hh>
#include <karabo/xms/Output.hh>

#include "CpuImage.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package xip
     */
    namespace xip {

        template <class TPix>
        class ImageFileWriter : public karabo::xms::Output< CpuImage<TPix> > {

        public:

            KARABO_CLASSINFO(ImageFileWriter, "File", "1.0")

            ImageFileWriter() {
            };

            virtual ~ImageFileWriter() {
            };

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Master)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::util;

                STRING_ELEMENT(expected)
                        .key("filename")
                        .description("Name of the file to be read")
                        .displayedName("Filename")
                        .displayType("Path")
                        .assignmentMandatory()
                        .commit();

                BOOL_ELEMENT(expected).key("addNumbers")
                        .description("If true, several calls to write will result in filenames that have a six-digit number appended to the filename, otherwise the file will be overwritten each time")
                        .displayedName("Add Numbers")
                        .assignmentOptional().defaultValue(false)
                        .commit();
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             *              */
            void configure(const karabo::util::Hash& input) {
                m_input = input;
                input.get("filename", m_filename);
                m_number = -1;
                if (input.get<bool>("addNumbers")) {
                    m_number = 0;
                }
            }

            void write(const CpuImage<TPix>& image) {
                try {
                    try {
                        image.getCImg().save(m_filename.string().c_str(), m_number);
                        if (m_number >= 0) m_number++;
                        return;
                    } catch (...) {
                        std::string extension = m_filename.extension().string().substr(1);
                        boost::to_lower(extension);
                        std::vector<std::string> keys = karabo::util::Factory<karabo::xms::Output<CpuImage<TPix> > >::getRegisteredKeys();

                        BOOST_FOREACH(std::string key, keys) {
                            std::string lKey(key);
                            boost::to_lower(lKey);
                            if (lKey == extension) {
                                boost::shared_ptr<karabo::xms::Output<CpuImage<TPix> > > in = karabo::xms::Output<CpuImage<TPix> >::create(m_input);
                                in->write(image);
                                return;
                            }
                        }
                        throw KARABO_IMAGE_TYPE_EXCEPTION("Can not read image of type \"" + extension + "\"");
                    }
                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_IO_EXCEPTION("Problems reading image " + m_filename.string()));
                }
            }

        private:

            karabo::util::Hash m_input;
            boost::filesystem::path m_filename;
            int m_number;
        };

    }
}

#endif	/* KARABO_XIP_ANYFORMATIMAGEWRITER_HH */
