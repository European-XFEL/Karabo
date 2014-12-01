/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XIP_IMAGEFILEREADER_HH
#define	KARABO_XIP_IMAGEFILEREADER_HH

#include <boost/filesystem.hpp>

#include <karabo/util/Configurator.hh>
#include <karabo/io/Input.hh>
#include <karabo/util/PathElement.hh>

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
        class ImageFileReader : public karabo::io::Input< CpuImage<TPix> > {

        public:

            KARABO_CLASSINFO(ImageFileReader, "ImageFile", "1.0")

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Master)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::util;

                PATH_ELEMENT(expected)
                        .key("filename")
                        .description("Name of the file to be read")
                        .displayedName("Filename")
                        .isInputFile()
                        .assignmentMandatory()
                        .commit();
            }

            ImageFileReader(const karabo::util::Hash& config) : karabo::io::Input< CpuImage<TPix> >(config) {
                m_filename = config.get<std::string>("filename");
                m_input = config;
            }
            
            virtual ~ImageFileReader() {
            };

            void read(CpuImage<TPix>& image, size_t idx = 0) {
                try {
                    CpuImage<TPix> tmp;
                    try {
                        tmp.getCImg().load(m_filename.c_str());
                        image.swap(tmp);
                        return;
                    } catch (...) {
                        std::string extension = boost::filesystem::path(m_filename).extension().string().substr(1);
                        boost::to_lower(extension);
                        std::vector<std::string> keys = karabo::util::Configurator<karabo::io::Input<CpuImage<TPix> > >::getRegisteredClasses();

                        BOOST_FOREACH(std::string key, keys) {
                            std::string lKey(key);
                            boost::to_lower(lKey);
                            if (lKey == extension) {
                                typename karabo::io::Input<CpuImage<TPix> >::Pointer in = karabo::io::Input<CpuImage<TPix> >::create(m_input);
                                in->read(tmp);
                                image.swap(tmp);
                                return;
                            }
                        }
                        throw KARABO_IMAGE_TYPE_EXCEPTION("Can not read image of type \"" + extension + "\"");
                    }
                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_IO_EXCEPTION("Problems reading image " + m_filename));
                }
            }

            bool canCompute() const {
                return boost::filesystem::exists(boost::filesystem::path(m_filename));
            }

            size_t size() {
                // TODO Work on this
                return 1;
            }

        private:

            std::string m_filename;
            karabo::util::Hash m_input;
        };

    }
}

#endif
