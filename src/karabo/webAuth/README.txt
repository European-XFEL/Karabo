######################################################################################
#
# How add a Web Service Client to C++ using Gsoap
#
#
# (note: if you want to use Karabo libraries they can be found in: $KARABO/extern/resources/gsoap/gsoap-2.8/gsoap)
######################################################################################

$> wsdl2h -o ClientAuthentication.h http://exfl-tb04:8080/XFELWebAuth/Authentication?WSDL

$> soapcpp2 -j -I/usr/share/gsoap/import ClientAuthentication.h

--> Add files stdsoap2.cpp / stdsoap2.h to project
--> Add libraries gsoap++ / gsoapck++ / gsoapssl
--> Add "-DWITH_NONAMESPACES" to stdsoap2.cpp

