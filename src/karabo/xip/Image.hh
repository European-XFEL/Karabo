/*
 * $Id$
 *
 * File:   Image.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 23, 2012, 12:42 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XIP_IMAGE_HH
#define	KARABO_XIP_IMAGE_HH

#include "Environment.hh"


namespace karabo {
    namespace xip {
        
        enum ImageType {
            CPU,
            GPU
        };

        template <class TPix>
        class Image {
            typedef typename boost::shared_ptr<AbstractImage<TPix> > AbstractImagePointer;
            typedef typename boost::shared_ptr<Environment<TPix> > EnvironmentPointer;

            static EnvironmentPointer m_cpuEnvironment;
            static EnvironmentPointer m_cudaEnvironment;
            AbstractImagePointer m_img;

        public:

            /***************************************
             *            Constructors             *
             ***************************************/

            Image(const int imageType) {
                if ( imageType == CPU) m_img = initCpu()->image();
                else m_img = initCuda()->image();
            }
            
            Image(const int imageType, const std::string& filename) {
                if (imageType == CPU) m_img = initCpu()->image(filename);
                else m_img = initCuda()->image(filename);
            }
            
            Image(const int imageType, const size_t dx, const size_t dy = 1, const size_t dz = 1) {
                if (imageType == CPU) m_img = initCpu()->image(dx, dy, dz);
                else m_img = initCuda()->image(dx, dy, dz);
            }

         

        private:

            const EnvironmentPointer& initCpu() {
                if (!m_cpuEnvironment) {
                    m_cpuEnvironment = Environment<TPix>::create("cpu");
                    m_cpuEnvironment->printInfo();
                }
                return m_cpuEnvironment;
            }

            const EnvironmentPointer& initCuda() {
                if (!m_cudaEnvironment) {
                    try {
                        m_cudaEnvironment = Environment<TPix>::create("cuda");
                        m_cudaEnvironment->printInfo();
                    } catch (karabo::util::Exception&) {
                        std::cout << "Falling back to CPU version" << std::endl;
                        m_cudaEnvironment = initCpu();
                    }
                }
                return m_cudaEnvironment;
            }

        }; 
        
        template<class TPix>
        typename Image<TPix>::EnvironmentPointer Image<TPix>::m_cpuEnvironment = typename Image<TPix>::EnvironmentPointer();
        
        template<class TPix>
        typename Image<TPix>::EnvironmentPointer Image<TPix>::m_cudaEnvironment = typename Image<TPix>::EnvironmentPointer();
        
    }
}



#endif	/* EXFEL_PACKAGENAME_IMAGE_HPP */

