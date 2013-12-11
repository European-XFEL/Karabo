######################################################################################
#
# Notes
#
# Note 01: After Karabo version 1.0 gsoap libraries can no longer be found at: $KARABO/extern/resources/gsoap/gsoap-2.8/gsoap
# Note 02: gSOAP Toolkit web page available at http://www.cs.fsu.edu/~engelen/soap.html
# Note 03: gSOAP download available at http://sourceforge.net/projects/gsoap2/files/gSOAP/
#
# Note 04: Current files generated with gSOAP version 2.8.17 (released at 2013-12-02)
######################################################################################


######################################################################################
#
# Current Web Service Server location
#
# HTTP  => http://exfl-tb04:8080/XFELWebAuth/Authentication?WSDL
# HTTPS => https://exfl-tb04:8181/XFELWebAuth/Authentication?WSDL
######################################################################################


######################################################################################
#
# How add a Web Service Client to C++ using Gsoap
#
######################################################################################

$> wsdl2h -o ClientAuthentication.h https://exfl-tb04:8181/XFELWebAuth/Authentication?WSDL

$> soapcpp2 -j -I/usr/share/gsoap/import ClientAuthentication.h

--> Add files stdsoap2.cpp / stdsoap2.h to project
--> Add libraries gsoap++ / gsoapck++ / gsoapssl
--> Add "-DWITH_NONAMESPACES" to stdsoap2.cpp


P.S. - For more information consult gsoap README.txt
######################################################################################




