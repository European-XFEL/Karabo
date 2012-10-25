/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XIP_IMAGEFILEREADER_HH
#define	KARABO_XIP_IMAGEFILEREADER_HH

#include <karabo/util/Factory.hh>

#include "Input.hh"
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
        class ImageFileReader : public Input< CpuImage<TPix> > {
        public:

            KARABO_CLASSINFO(ImageFileReader, "File", "1.0")

            ImageFileReader() {
            };

            virtual ~ImageFileReader() {
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
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             *              */
            void configure(const karabo::util::Hash& input) {
                input.get("filename", m_filename);
                m_input = input;
            }

            void read(CpuImage<TPix>& image) {
                try {
                    CpuImage<TPix> tmp;
                    try {
                        tmp.getCImg().load(m_filename.string().c_str());
                        image.swap(tmp);
                        return;
                    } catch (...) {
                        std::string extension = m_filename.extension().string().substr(1);
                        boost::to_lower(extension);
                        std::vector<std::string> keys = karabo::util::Factory<Input<CpuImage<TPix> > >::getRegisteredKeys();

                        BOOST_FOREACH(std::string key, keys) {
                            std::string lKey(key);
                            boost::to_lower(lKey);
                            if (lKey == extension) {
                                boost::shared_ptr<Input<CpuImage<TPix> > > in = Input<CpuImage<TPix> >::create(m_input);
                                in->read(tmp);
                                image.swap(tmp);
                                return;
                            }
                        }
                        throw IMAGE_TYPE_EXCEPTION("Can not read image of type \"" + extension + "\"");
                    }
                } catch (...) {
                    RETHROW_AS(IO_EXCEPTION("Problems reading image " + m_filename.string()));
                }
            }
            
            karabo::util::Hash getIOStatus() const {
                return karabo::util::Hash();
            }
            
            bool canRead() const {
                return boost::filesystem::exists(m_filename);
            }
            
            void setEndOfStream() const {
                
            }
            
            
        private:

            boost::filesystem::path m_filename;
            karabo::util::Hash m_input;
        };

    }
}

#endif
