/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_XIP_PHOTONIMAGEFILEWRITER_HH
#define	EXFEL_XIP_PHOTONIMAGEFILEWRITER_HH

#include <fstream>

#include <exfel/util/Factory.hh>
#include "Output.hh"

#include "CpuImageList.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

    /**
     * Namespace for package xip
     */
    namespace xip {

        template <class TPix>
        class PhotonImageFileWriter /*: public Output< CpuImage<TPix> >*/ {

        public:

            EXFEL_CLASSINFO(PhotonImageFileWriter, "PhotonFile", "1.0")

            PhotonImageFileWriter() {
            };

            virtual ~PhotonImageFileWriter() {
            };

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Master)
             */
            static void expectedParameters(exfel::util::Schema& expected) {
                using namespace exfel::util;

                PATH_ELEMENT(expected).key("filename")
                        .description("Name of the file to be read")
                        .displayedName("Filename")
                        .assignmentOptional().defaultValue("photons.dat")
                        .commit();

                FLOAT_ELEMENT(expected).key("meanTotalIntensity")
                        .description("Mean total intensity")
                        .displayedName("Mean Intensity")
                        .assignmentOptional().defaultValue(0.0)
                        .commit();
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             *              */
            void configure(const exfel::util::Hash& input) {
                input.get("filename", m_filename);
                m_os.open(m_filename.string().c_str(), std::ios::trunc);
            }

            void write(const CpuImage<TPix>& image) {
                try {
                    //std::ofstream m_os(m_filename.string().c_str(), std::ios::trunc);
                    std::vector<size_t> ones;
                    std::vector<std::pair<size_t, TPix> > multis;

                    // Loop all images
                    for (size_t i = 0; i < image.size(); ++i) {
                        if (image[i] == 1) {
                            ones.push_back(i);
                        } else if (image[i] > 1) {
                            multis.push_back(std::pair<size_t, TPix > (i, image[i]));
                        }
                    }
                    addDiffractionImage(ones, multis);
                } catch (...) {
                    RETHROW_AS(IO_EXCEPTION("Problems writing image " + m_filename.string()));
                }
            }

            void update() {
                m_os.close();
            }

        private: // functions

            void addDiffractionImage(const std::vector<size_t>& ones, const std::vector<std::pair<size_t, TPix> >& multis) {
                m_os << ones.size() << std::endl;
                for (size_t i = 0; i < ones.size(); ++i) {
                    m_os << ones[i] << " ";
                }
                m_os << std::endl;
                m_os << multis.size() << std::endl;
                for (size_t i = 0; i < multis.size(); ++i) {
                    m_os << multis[i].first << " " << multis[i].second << " ";
                }
                m_os << std::endl << std::endl;
            }

        private: // members

            std::ofstream m_os;
            boost::filesystem::path m_filename;
            float m_meanTotalIntensity;
        };

    }
}

#endif	
