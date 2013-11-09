/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */

#ifndef __ESP_ERRORS_HPP__
#define __ESP_ERRORS_HPP__


namespace EspCoreErrors
{
    const unsigned int Base = 1000;
    const unsigned int FEATURE_DEPRECATED           = Base+1;
    const unsigned int MissingConfigurationFile     = Base+2;  // Esp.xml config file not found
    const unsigned int ConfigurationFileEntryError  = Base+3;  // Required entry in esp.xml missing or invalid
    const unsigned int EsdlFileError                = Base+4;  // Required ESDL configuration file file missing or incorrect
    const unsigned int MethodParameterError         = Base+5;  // Something wrong with a parameter passed into the method.
    const unsigned int DatabaseError                = Base+6;
    const unsigned int InvalidGLBPurpose            = Base+7;
    const unsigned int InvalidDLPurpose             = Base+8;
    const unsigned int RequestFormatError           = Base+9;
    const unsigned int ResponseFormatError          = Base+10;
    const unsigned int XmlParsingError              = Base+11;
    const unsigned int HttpStatusError              = Base+12;
    const unsigned int XslProcessorError            = Base+13;
    const unsigned int XslTransformError            = Base+14;
    const unsigned int SoapError                    = Base+15;
    const unsigned int MemoryAllocationError        = Base+16;
    const unsigned int NotEnoughInputCriteriaError  = Base+17;
    const unsigned int MissingValueError            = Base+18;
}

namespace MySQLErrors
{
    const unsigned int Base = EspCoreErrors::Base+1800;       //2800 -- pls don't change this....the web is coded to expect the following codes
    const unsigned int PrepedStmtAllocHandleFailed  = Base+0; //"Can't allocate a handle for the prepared statement."
    const unsigned int PrepedStmtMissing            = Base+1; //"The prepared statement is NULL."
    const unsigned int PrepedStmtBindParamsFailed   = Base+2; //"Binding of the parameters to the prepared statement failed."
    const unsigned int PrepedStmtBindColumnsFailed  = Base+3; //"Binding of the columns to the prepared statement failed."
    const unsigned int PrepedStmtExecuteFailed      = Base+4; //"Executing the prepared statement failed."
    const unsigned int PrepedStmtStoreResultFailed  = Base+5; //"Storing fetched records on the client for the prepared statement failed."
    const unsigned int MismatchingNumOfParams       = Base+6; //"The number of params to be set is different from the number of params in the prepared statement."
    const unsigned int ConnectionTimedOut           = Base+7; //The connection to mysql timed out
}

namespace WsGatewayErrors
{
    const unsigned int Base = EspCoreErrors::Base+200;   //1200 -- pls don't change this....
    const unsigned int MissingConfiguration = Base+0;
    const unsigned int MissingUserName      = Base+1;
    const unsigned int MissingPassword      = Base+2;
    const unsigned int MissingOptions       = Base+3;
    const unsigned int MissingUrl           = Base+4;
    const unsigned int InvalidUrl           = Base+5;
    const unsigned int NoResponse           = Base+6;
}

namespace WsAccurintErrors
{
    const unsigned int Base = EspCoreErrors::Base+1005;   //2005 -- pls don't change this....the web is coded to expect the following codes
    const unsigned int RealTimeInvalidUse       = Base+0; //"Access to realtime data is not allowed for your intended use."
    const unsigned int RealTimeInvalidState     = Base+1; //"Access to realtime search is not allowed under the DPPA guidelines for your intended use for that state."
    const unsigned int RealTimeMissingStateZip  = Base+2; //"A valid state or zip must be specified for realtime data."
    const unsigned int RealTimeMissingName      = Base+3; //"A person or company name must be specified for realtime data."
    const unsigned int RealTimeMissingAddress   = Base+4; //"Street address with City+State or Zip must be specified for realtime data."
}


#endif //__ESP_ERRORS_HPP__
