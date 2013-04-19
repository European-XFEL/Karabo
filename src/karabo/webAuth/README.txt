######################################################################################
#
# How add a Web Service Client to C++ using Gsoap
#
######################################################################################

$> wsdl2h -o ClientAuthentication.h http://exflpcx18262:8080/XFELAuthWebService/Authentication?WSDL

$> soapcpp2 -j -I/usr/share/gsoap/import auth.h

--> Add these functions to soap.C.cpp file:
SOAP_FMAC3 const char * SOAP_FMAC4 soap_check_faultsubcode(struct soap *soap)
{
        soap_fault(soap);
        if (soap->version == 2)
        {       if (soap->fault->SOAP_ENV__Code && soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode && soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode)
                        return soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode->SOAP_ENV__Value;
                return NULL;
        }
        return soap->fault->faultcode;
}

SOAP_FMAC3 const char * SOAP_FMAC4 soap_check_faultdetail(struct soap *soap)
{
        soap_fault(soap);
        if (soap->version == 2 && soap->fault->SOAP_ENV__Detail)
                return soap->fault->SOAP_ENV__Detail->__any;
        if (soap->fault->detail)
                return soap->fault->detail->__any;
        return NULL;
}


--> Add files stdsoap2.cpp / stdsoap2.h to project
--> Add libraries gsoap++ / gsoapck++ / gsoapssl
--> Add "-DWITH_NONAMESPACES" to stdsoap2.cpp