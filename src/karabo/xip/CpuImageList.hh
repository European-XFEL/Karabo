/*
 * $Id$
 *
 * File:   CpuImageList.hh
 * Author: <bukhard.heisen@xfel.eu>
 *
 * Created on November 23, 2011, 5:59 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_PACKAGENAME_CPUIMAGELIST_HH
#define	KARABO_PACKAGENAME_CPUIMAGELIST_HH

#include <deque>

#include "CpuImage.hh"

namespace karabo {
    namespace xip {

        template <class TPix>
        class CpuImageList : public std::deque< CpuImage<TPix> > {
        public:

            CpuImageList() : std::deque<CpuImage<TPix> >() {
            }

            CpuImageList(size_t n, const CpuImage<TPix>& image = CpuImage<TPix>()) : std::deque< CpuImage<TPix> >(n, image) {
            }

            CpuImageList(const TPix * const dataBuffer, const unsigned int n, const unsigned int dx, const unsigned int dy, const unsigned int dz) {

            }

            CpuImageList(const std::vector<TPix>& dataBuffer, const unsigned int n, const unsigned int dx, const unsigned int dy, const unsigned int dz) {
            }

            void createContiguousBuffer(std::vector<char>& buffer) const {
                if (!this->empty()) {
                    // Assumption and convention: all image are of same size!!
                    size_t imageByteSize = this->front().byteSize();
                    size_t fullSize = this->size() * imageByteSize;
                    buffer.resize(fullSize);
                    size_t offset = 0;
                    for (typename std::deque< CpuImage<TPix> >::const_iterator it = this->begin(); it != this->end(); ++it) {
                        std::memcpy(&buffer[offset], it->pixelPointer(), imageByteSize);
                        offset += imageByteSize;
                    }
                }
            }
            
            void createMetaDataHeader(karabo::util::Hash& header) const {
                if (!this->empty()) {
                    const CpuImage<TPix>& first = this->front();
                    header.set("dimX", (unsigned int)first.dimX());
                    header.set("dimY", (unsigned int)first.dimY());
                    header.set("dimZ", (unsigned int)first.dimZ());
                    header.set("nImages", (unsigned int) this->size());
                    header.set("pixelType", (int)karabo::util::Types::getTypeAsId<TPix>());
                }
            }

        };
        
        typedef CpuImageList<int> CpuImgIList;
        typedef CpuImageList<double> CpuImgDList;
       
        typedef Input<CpuImgIList> InputCpuImgIList;
        typedef Output<CpuImgIList> OutputCpuImgIList;
        
        typedef Input<CpuImgDList> InputCpuImgDList;
        typedef Output<CpuImgDList> OutputCpuImgDList;
        

    }
}



#endif	/* KARABO_PACKAGENAME_CPUIMAGELIST_HH */

