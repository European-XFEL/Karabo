/* 
 * Author: <krzysztof.wrona@xfel.eu> 
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARATHON_DIMSWRAP_HH
#define	KARATHON_DIMSWRAP_HH

#include <boost/python.hpp>

#include <karathon/Wrapper.hh>
#include <karabo/util/Dims.hh>



namespace karathon {

    class DimsWrap : public karabo::util::Dims {
    public:

        DimsWrap() : karabo::util::Dims() {
        }

        DimsWrap(const std::vector<unsigned long long>& vec) : karabo::util::Dims(vec) {
        }

        DimsWrap(bp::list list) : karabo::util::Dims(convertFromListToVector(list)) {
        }

        DimsWrap(unsigned long long xSize) : karabo::util::Dims(xSize) {
        }

        DimsWrap(unsigned long long xSize, unsigned long long ySize) : karabo::util::Dims(xSize, ySize) {
        }

        DimsWrap(unsigned long long xSize, unsigned long long ySize, unsigned long long zSize) : karabo::util::Dims(xSize, ySize, zSize) {
        }

        DimsWrap(unsigned long long x1Size, unsigned long long x2Size, unsigned long long x3Size, unsigned long long x4Size) :
        karabo::util::Dims(x1Size, x2Size, x3Size, x4Size) {
        }

        bp::object toVectorPy() {
            return Wrapper::fromStdVectorToPyList(this->toVector());
        }

        bp::object toArrayPy() {
            return Wrapper::fromStdVectorToPyArray(this->toVector());
        }

    private:

        std::vector<unsigned long long> convertFromListToVector(bp::list& dims) {
            std::vector<unsigned long long> vec(bp::len(dims), 0);
            for (int i = 0; i < bp::len(dims); ++i) {
                vec[i] = bp::extract<unsigned long long>(dims[i]);
            }
            return vec;
        }
    };

}


#endif	/* KARATHON_DIMSWRAP_HH */

