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

#include "jlog.hpp"
#include "dbfieldmap.hpp"

void DBFieldMap::loadMappings(IPropertyTree& fieldList)
{
    Owned<IPropertyTreeIterator> itr = fieldList.getElements("Field");
    ForEach(*itr)
    {
        IPropertyTree &map = itr->query();
        const char* name = map.queryProp("@name");
        const char* mapTo = map.queryProp("@mapto");
        if (!name || !*name)
            continue;

        StringBuffer oldValue, newValue;
        oldValue.append(name).toLowerCase();
        mapNames.append(name);
        if (!mapTo || !*mapTo)
            map.setProp("@mapto", name);
        else
        {
            newValue.append(mapTo).toLowerCase();
            fieldMapping[oldValue.toCharArray()] = newValue.toCharArray();
        }
    }
    mapFields.set(&fieldList);
}

void DBFieldMap::mappingField(const char* field, StringBuffer& returnField)
{
    StringBuffer lowercaseField(field);
    lowercaseField.toLowerCase();

    std::string mappedField = fieldMapping[lowercaseField.str()];
    if(mappedField.size() > 0)
        returnField.set(mappedField.c_str());
    else
        returnField.set(lowercaseField.str());
}

bool DBFieldMap::formatField(const char* field,const char* value, StringBuffer& returnField, MemoryBuffer& returnValue, bool isFormatField)
{
    if(!field || !*field || !value || !*value)
        return false;

    //If we dont want to formatField every time we can set isFormatField to false.
    if(isFormatField || !returnField.length())
        mappingField(field,returnField);

    VStringBuffer xPath("Field[@mapto=\"%s\"]",returnField.str());
    IPropertyTree* pField = mapFields->queryBranch(xPath.str());
    if(!pField)
    {
        DBGLOG("DB field %s not found in configuration", returnField.str());
        return false;
    }

    returnValue.append(strlen(value),value);

    const char* typeVal   = pField->queryProp("@type");
    if (typeVal && !streq(typeVal,"raw") && !formatField(typeVal, returnValue))
    {
        DBGLOG("Cannot format %s", typeVal);
        return false;
    }

    return true;
}

bool DBFieldMap::formatField(const char* fieldType, MemoryBuffer& returnValue)
{
    if(!fieldType || !*fieldType)
        return true;

    StringBuffer tmp;
    if(strieq(fieldType,"string"))
    {
        clean(returnValue);
        tmp.append(returnValue.length(), returnValue.toByteArray());
        if (tmp.trim().length() == 0)
            return false;

        tmp.insert(0,"'");
        tmp.appendf("%s","'");
        returnValue.resetBuffer();
        returnValue.append(tmp.length(),(void*)tmp.toCharArray());
    }
    else if(strieq(fieldType,"numeric"))
    {
        tmp.append(returnValue.length(), returnValue.toByteArray());
        if (tmp.trim().length() == 0)
            return false;

        returnValue.resetBuffer();
        returnValue.append(tmp.length(),(void*)tmp.toCharArray());
    }
    //else // do nothing
    return true;
}

void DBFieldMap::clean(MemoryBuffer& name)
{
    MemoryBuffer tmp;
    const char* str = name.toByteArray();
    int length = name.length();
    for(int i = 0; i < length; i++)
    {
        unsigned char c = str[i];
        if(c == '\t' || c == '\n' || c== '\r')
            tmp.append(' ');
        else if(c < 32 || c > 126)
            tmp.append('?');
        else
            tmp.append(c);
    }

    int buflen = tmp.length();
    void * buf = tmp.detach();
    name.setBuffer(buflen, buf, true);
}
