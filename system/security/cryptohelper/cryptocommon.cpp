/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2018 HPCC Systems®.

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

#if defined(_USE_OPENSSL) && !defined(_WIN32)

#include "jliball.hpp"
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include "cryptocommon.hpp"


static void addAlgorithms()
{
    OpenSSL_add_all_algorithms();
}

MODULE_INIT(INIT_PRIORITY_STANDARD)
{
    addAlgorithms();
    return true;
}
MODULE_EXIT()
{
}


namespace cryptohelper
{

IException *makeEVPExceptionVA(int code, const char *format, va_list args)
{
    StringBuffer formatMessage;
    char *evpErr = formatMessage.reserve(120);
    ERR_error_string_n(ERR_get_error(), evpErr, 120);
    formatMessage.setLength(strlen(evpErr));
    formatMessage.append(" - ").append(format);
    return makeStringExceptionVA(code, formatMessage, args);
}

IException *makeEVPException(int code, const char *msg)
{
    StringBuffer formatMessage;
    char *evpErr = formatMessage.reserve(120);
    ERR_error_string_n(ERR_get_error(), evpErr, 120);
    formatMessage.setLength(strlen(evpErr));
    formatMessage.append(" - ").append(msg);
    return makeStringException(code, formatMessage);
}

IException *makeEVPExceptionV(int code, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    IException *e = makeEVPExceptionVA(code, format, args);
    va_end(args);
    return e;
}

void throwEVPException(int code, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    IException *e = makeEVPExceptionVA(code, format, args);
    va_end(args);
    throw e;
}

} // end of namespace cryptohelper

#endif // #if defined(_USE_OPENSSL) && !defined(_WIN32)
