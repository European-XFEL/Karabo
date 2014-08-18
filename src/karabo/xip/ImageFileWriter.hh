/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XIP_ANYFORMATIMAGEWRITER_HH
#define	KARABO_XIP_ANYFORMATIMAGEWRITER_HH

#include <boost/filesystem.hpp>

#include <karabo/util/Configurator.hh>
#include <karabo/io/Output.hh>
#include <karabo/util/PathElement.hh>
#include <karabo/util/SimpleElement.hh>

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
        class ImageFileWriter : public karabo::io::Output< CpuImage<TPix> > {

            karabo::util::Hash m_input;
            boost::filesystem::path m_filename;
            int m_number;
            
        public:

            KARABO_CLASSINFO(ImageFileWriter, "ImageFile", "1.0")

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Master)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::util;

                PATH_ELEMENT(expected)
                        .key("filename")
                        .description("Name of the file to be written")
                        .displayedName("Filename")
                        .isOutputFile()
                        .assignmentMandatory()
                        .commit();
            }

            ImageFileWriter(const karabo::util::Hash& config) : karabo::io::Output< CpuImage<TPix> >(config) {
                m_input = config;
                m_filename = config.get<std::string>("filename");
                m_number = -1;
                if (this->m_appendModeEnabled) {
                    m_number = 0;
                }
            }

            virtual ~ImageFileWriter() {
            };

            void write(const CpuImage<TPix>& image) {
                try {
                    try {
                        image.getCImg().save(m_filename.string().c_str(), m_number);
                        if (m_number >= 0) m_number++;
                        return;
                    } catch (...) {
                        std::string extension = m_filename.extension().string().substr(1);
                        boost::to_lower(extension);
                        std::vector<std::string> keys = karabo::util::Configurator<karabo::io::Output<CpuImage<TPix> > >::getRegisteredClasses();

                        BOOST_FOREACH(std::string key, keys) {
                            std::string lKey(key);
                            boost::to_lower(lKey);
                            if (lKey == extension) {
                                typename karabo::io::Output<CpuImage<TPix> >::Pointer in = karabo::io::Output<CpuImage<TPix> >::create(m_input);
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
        };
    }
}

#endif
