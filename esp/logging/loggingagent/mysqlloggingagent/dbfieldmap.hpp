/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2014 HPCC Systems.

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

#ifndef _DBFIELDMAP_HPP__
#define _DBFIELDMAP_HPP__

#pragma warning (disable : 4786)

#include "jiface.hpp"
#include "jstring.hpp"
#include "jptree.hpp"
#include <map>
#include <string>

class DBFieldMap : public CInterface
{
    std::map<std::string, std::string>  fieldMapping;
    Owned<IPropertyTree>                mapFields;
    StringArray                         mapNames;

    bool formatField(const char* fieldType, MemoryBuffer& returnValue);
    void clean(MemoryBuffer& name);
public:
    IMPLEMENT_IINTERFACE;
    DBFieldMap() {};
    virtual ~DBFieldMap() {};

    void loadMappings(IPropertyTree& cfg);
    StringArray& getMapNames() {return mapNames;};
    bool formatField(const char* field, const char* value,StringBuffer& returnField,MemoryBuffer& returnValue, bool isFormatField = true);
    void mappingField(const char* field, StringBuffer& returnField);
};

#endif // !_DBFIELDMAP_HPP__
