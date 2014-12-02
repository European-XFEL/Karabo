/* 
 * File:   DetectorGeometryWrap.hh
 * Author: haufs
 *
 * Created on July 30, 2014, 3:14 PM
 */

#ifndef DETECTORGEOMETRYWRAP_HH
#define	DETECTORGEOMETRYWRAP_HH

#include <boost/python.hpp>

#include <karathon/Wrapper.hh>
#include <karabo/util/DetectorGeometry.hh>
#include <karabo/util/Hash.hh>



namespace karathon {

    class DetectorGeometryWrap : public karabo::util::DetectorGeometry {

    public:
        
        DetectorGeometryWrap() : karabo::util::DetectorGeometry() {
        }
        
        DetectorGeometryWrap(karabo::util::Hash h) : karabo::util::DetectorGeometry( h ){            
        }

        bp::object getOffsetsAsArray() {
                return Wrapper::fromStdVectorToPyArray(this->getOffsets());
        }

        bp::object getOffsetsAsList() {
                return Wrapper::fromStdVectorToPyList(this->getOffsets());
        }
        
        bp::object getRotationsAsArray() {
                return Wrapper::fromStdVectorToPyArray(this->getRotations());
        }

        bp::object getRotationsAsList() {
                return Wrapper::fromStdVectorToPyList(this->getRotations());
        }
        
        /*static const DetectorGeometryWrap & startSubAssemblyPy(DetectorGeometry& self){
            
            return *dynamic_cast<DetectorGeometryWrap*>(&(self.startSubAssembly()));
            //return self.startSubAssembly();
        }
        
        static const DetectorGeometryWrap & endSubAssemblyPy(DetectorGeometry& self){
            
            return *dynamic_cast<DetectorGeometryWrap*>(&(self.endSubAssembly()));
            //return self.endSubAssembly();
        }*/
        
        static karabo::util::Hash toHashPy(DetectorGeometry& self) {
            return self.toHash();
        }
        
        static void setOffsetsPy(DetectorGeometry& self, bp::object a, bp::object b, bp::object c){
            self.setOffsets(bp::extract<double>(a), bp::extract<double>(b), bp::extract<double>(c));
        }
        
        static void setRotationsPy(DetectorGeometry& self, bp::object a, bp::object b, bp::object c){
            self.setRotations(bp::extract<double>(a), bp::extract<double>(b), bp::extract<double>(c));
        }
 
        
    };

} 

#endif	/* DETECTORGEOMETRYWRAP_HH */

