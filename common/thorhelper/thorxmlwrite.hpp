/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems®.

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

#ifndef THORXMLWRITE_HPP
#define THORXMLWRITE_HPP

#ifdef _WIN32
 #ifdef THORHELPER_EXPORTS
  #define thorhelper_decl __declspec(dllexport)
 #else
  #define thorhelper_decl __declspec(dllimport)
 #endif
#else
 #define thorhelper_decl
#endif

#include "eclhelper.hpp"
#include "jptree.hpp"
#include "thorhelper.hpp"

interface IXmlStreamFlusher
{
    virtual void flushXML(StringBuffer &current, bool isClose) = 0;
};

interface IXmlWriterExt : extends IXmlWriter
{
    virtual IXmlWriterExt & clear() = 0;
    virtual size32_t length() const = 0;
    virtual const char *str() const = 0;
    virtual IInterface *saveLocation() const = 0;
    virtual void rewindTo(IInterface *location) = 0;
    virtual void outputNumericString(const char *field, const char *fieldName) = 0;
};

class thorhelper_decl CommonXmlPosition : public CInterface, implements IInterface
{
public:
    IMPLEMENT_IINTERFACE;

    CommonXmlPosition(size32_t _pos, unsigned _indent, unsigned _nestLimit, bool _tagClosed, bool _needDelimiter) :
        pos(_pos), indent(_indent), nestLimit(_nestLimit), tagClosed(_tagClosed), needDelimiter(_needDelimiter)
    {}

public:
    size32_t pos = 0;
    unsigned indent = 0;
    unsigned nestLimit = 0;
    bool tagClosed = false;
    bool needDelimiter = false;
};

class thorhelper_decl CommonXmlWriter : implements IXmlWriterExt, public CInterface
{
public:
    CommonXmlWriter(unsigned _flags, unsigned initialIndent=0,  IXmlStreamFlusher *_flusher=NULL);
    ~CommonXmlWriter();
    IMPLEMENT_IINTERFACE;

    void outputBeginNested(const char *fieldName, bool nestChildren, bool doIndent);
    void outputEndNested(const char *fieldName, bool doIndent);

    virtual void outputInlineXml(const char *text){closeTag(); out.append(text); flush(false);} //for appending raw xml content
    virtual void outputQuoted(const char *text);
    virtual void outputQString(unsigned len, const char *field, const char *fieldName);
    virtual void outputString(unsigned len, const char *field, const char *fieldName);
    virtual void outputBool(bool field, const char *fieldName);
    virtual void outputData(unsigned len, const void *field, const char *fieldName);
    virtual void outputInt(__int64 field, unsigned size, const char *fieldName);
    virtual void outputUInt(unsigned __int64 field, unsigned size, const char *fieldName);
    virtual void outputReal(double field, const char *fieldName);
    virtual void outputDecimal(const void *field, unsigned size, unsigned precision, const char *fieldName);
    virtual void outputUDecimal(const void *field, unsigned size, unsigned precision, const char *fieldName);
    virtual void outputUnicode(unsigned len, const UChar *field, const char *fieldName);
    virtual void outputUtf8(unsigned len, const char *field, const char *fieldName);
    virtual void outputBeginDataset(const char *dsname, bool nestChildren);
    virtual void outputEndDataset(const char *dsname);
    virtual void outputBeginNested(const char *fieldName, bool nestChildren);
    virtual void outputEndNested(const char *fieldName);
    virtual void outputBeginArray(const char *fieldName){}; //repeated elements are inline for xml
    virtual void outputEndArray(const char *fieldName){};
    virtual void outputSetAll();
    virtual void outputXmlns(const char *name, const char *uri);

    //IXmlWriterExt
    virtual IXmlWriterExt & clear();
    virtual unsigned length() const                                 { return out.length(); }
    virtual const char * str() const                                { return out.str(); }
    virtual IInterface *saveLocation() const
    {
        if (flusher)
            throwUnexpected();

        return new CommonXmlPosition(length(), indent, nestLimit, tagClosed, false);
    }
    virtual void rewindTo(IInterface *saved)
    {
        if (flusher)
            throwUnexpected();

        CommonXmlPosition *position = dynamic_cast<CommonXmlPosition *>(saved);
        if (!position)
            return;
        if (position->pos < out.length())
        {
            out.setLength(position->pos);
            tagClosed = position->tagClosed;
            indent = position->indent;
            nestLimit = position->nestLimit;
        }
    }

    virtual void outputNumericString(const char *field, const char *fieldName)
    {
        outputCString(field, fieldName);
    }

protected:
    bool checkForAttribute(const char * fieldName);
    void closeTag();
    inline void flush(bool isClose)
    {
        if (flusher)
            flusher->flushXML(out, isClose);
    }

protected:
    IXmlStreamFlusher *flusher;
    StringBuffer out;
    unsigned flags;
    unsigned indent;
    unsigned nestLimit;
    bool tagClosed;
};

class thorhelper_decl CommonJsonWriter : implements IXmlWriterExt, public CInterface
{
public:
    CommonJsonWriter(unsigned _flags, unsigned initialIndent=0,  IXmlStreamFlusher *_flusher=NULL);
    ~CommonJsonWriter();
    IMPLEMENT_IINTERFACE;

    void checkDelimit(int inc=0);
    void checkFormat(bool doDelimit, bool needDelimiter=true, int inc=0);
    void prepareBeginArray(const char *fieldName);

    virtual void outputInlineXml(const char *text) //for appending raw xml content
    {
        if (text && *text)
            outputUtf8(strlen(text), text, "xml");
    }
    virtual void outputQuoted(const char *text);
    virtual void outputQString(unsigned len, const char *field, const char *fieldName);
    virtual void outputString(unsigned len, const char *field, const char *fieldName);
    virtual void outputBool(bool field, const char *fieldName);
    virtual void outputData(unsigned len, const void *field, const char *fieldName);
    virtual void outputInt(__int64 field, unsigned size, const char *fieldName);
    virtual void outputUInt(unsigned __int64 field, unsigned size, const char *fieldName);
    virtual void outputReal(double field, const char *fieldName);
    virtual void outputDecimal(const void *field, unsigned size, unsigned precision, const char *fieldName);
    virtual void outputUDecimal(const void *field, unsigned size, unsigned precision, const char *fieldName);
    virtual void outputUnicode(unsigned len, const UChar *field, const char *fieldName);
    virtual void outputUtf8(unsigned len, const char *field, const char *fieldName);
    virtual void outputBeginDataset(const char *dsname, bool nestChildren);
    virtual void outputEndDataset(const char *dsname);
    virtual void outputBeginNested(const char *fieldName, bool nestChildren);
    virtual void outputEndNested(const char *fieldName);
    virtual void outputBeginArray(const char *fieldName);
    virtual void outputEndArray(const char *fieldName);
    virtual void outputSetAll();
    virtual void outputXmlns(const char *name, const char *uri){}
    virtual void outputNumericString(const char *field, const char *fieldName);

    //IXmlWriterExt
    virtual IXmlWriterExt & clear();
    virtual unsigned length() const                                 { return out.length(); }
    virtual const char * str() const                                { return out.str(); }
    virtual void rewindTo(unsigned int prevlen)                     { if (prevlen < out.length()) out.setLength(prevlen); }
    virtual IInterface *saveLocation() const
    {
        if (flusher)
            throwUnexpected();

        return new CommonXmlPosition(length(), indent, nestLimit, false, needDelimiter);
    }
    virtual void rewindTo(IInterface *saved)
    {
        if (flusher)
            throwUnexpected();

        CommonXmlPosition *position = dynamic_cast<CommonXmlPosition *>(saved);
        if (!position)
            return;
        if (position->pos < out.length())
        {
            out.setLength(position->pos);
            needDelimiter = position->needDelimiter;
            indent = position->indent;
            nestLimit = position->nestLimit;
        }
    }

    void outputBeginRoot(){out.append('{');}
    void outputEndRoot(){out.append('}');}

protected:
    inline void flush(bool isClose)
    {
        if (flusher)
            flusher->flushXML(out, isClose);
    }

    class CJsonWriterItem : public CInterface
    {
    public:
        CJsonWriterItem(const char *_name) : name(_name), depth(0){}

        StringAttr name;
        unsigned depth;
    };

    const char *checkItemName(CJsonWriterItem *item, const char *name, bool simpleType=true);
    const char *checkItemName(const char *name, bool simpleType=true);
    const char *checkItemNameBeginNested(const char *name);
    const char *checkItemNameEndNested(const char *name);
    bool checkUnamedArrayItem(bool begin);


    IXmlStreamFlusher *flusher;
    CIArrayOf<CJsonWriterItem> arrays;
    StringBuffer out;
    unsigned flags;
    unsigned indent;
    unsigned nestLimit;
    bool needDelimiter;
};

thorhelper_decl StringBuffer &buildJsonHeader(StringBuffer  &header, const char *suppliedHeader, const char *rowTag);
thorhelper_decl StringBuffer &buildJsonFooter(StringBuffer  &footer, const char *suppliedFooter, const char *rowTag);

//Writes type encoded XML strings  (xsi:type="xsd:string", xsi:type="xsd:boolean" etc)
class thorhelper_decl CommonEncodedXmlWriter : public CommonXmlWriter
{
public:
    CommonEncodedXmlWriter(unsigned _flags, unsigned initialIndent=0, IXmlStreamFlusher *_flusher=NULL);

    virtual void outputString(unsigned len, const char *field, const char *fieldName);
    virtual void outputBool(bool field, const char *fieldName);
    virtual void outputData(unsigned len, const void *field, const char *fieldName);
    virtual void outputInt(__int64 field, unsigned size, const char *fieldName);
    virtual void outputUInt(unsigned __int64 field, unsigned size, const char *fieldName);
    virtual void outputReal(double field, const char *fieldName);
    virtual void outputDecimal(const void *field, unsigned size, unsigned precision, const char *fieldName);
    virtual void outputUDecimal(const void *field, unsigned size, unsigned precision, const char *fieldName);
    virtual void outputUnicode(unsigned len, const UChar *field, const char *fieldName);
    virtual void outputUtf8(unsigned len, const char *field, const char *fieldName);
};

//Writes all encoded DATA fields as base64Binary
class thorhelper_decl CommonEncoded64XmlWriter : public CommonEncodedXmlWriter
{
public:
    CommonEncoded64XmlWriter(unsigned _flags, unsigned initialIndent=0, IXmlStreamFlusher *_flusher=NULL);

    virtual void outputData(unsigned len, const void *field, const char *fieldName);
};

enum XMLWriterType{WTStandard, WTEncoding, WTEncodingData64, WTJSON} ;
thorhelper_decl CommonXmlWriter * CreateCommonXmlWriter(unsigned _flags, unsigned initialIndent=0, IXmlStreamFlusher *_flusher=NULL, XMLWriterType xmlType=WTStandard);
thorhelper_decl IXmlWriterExt * createIXmlWriterExt(unsigned _flags, unsigned initialIndent=0, IXmlStreamFlusher *_flusher=NULL, XMLWriterType xmlType=WTStandard);

class thorhelper_decl SimpleOutputWriter : implements IXmlWriter, public CInterface
{
    void outputFieldSeparator();
    bool separatorNeeded;
public:
    SimpleOutputWriter();
    IMPLEMENT_IINTERFACE;

    SimpleOutputWriter & clear();
    unsigned length() const                                 { return out.length(); }
    const char * str() const                                { return out.str(); }

    virtual void outputQuoted(const char *text);
    virtual void outputQString(unsigned len, const char *field, const char *fieldName);
    virtual void outputString(unsigned len, const char *field, const char *fieldName);
    virtual void outputBool(bool field, const char *fieldName);
    virtual void outputData(unsigned len, const void *field, const char *fieldName);
    virtual void outputReal(double field, const char *fieldName);
    virtual void outputDecimal(const void *field, unsigned size, unsigned precision, const char *fieldName);
    virtual void outputUDecimal(const void *field, unsigned size, unsigned precision, const char *fieldName);
    virtual void outputUnicode(unsigned len, const UChar *field, const char *fieldName);
    virtual void outputUtf8(unsigned len, const char *field, const char *fieldName);
    virtual void outputBeginNested(const char *fieldName, bool nestChildren);
    virtual void outputEndNested(const char *fieldName);
    virtual void outputBeginDataset(const char *dsname, bool nestChildren){}
    virtual void outputEndDataset(const char *dsname){}
    virtual void outputBeginArray(const char *fieldName){}
    virtual void outputEndArray(const char *fieldName){}
    virtual void outputSetAll();
    virtual void outputInlineXml(const char *text){} //for appending raw xml content
    virtual void outputXmlns(const char *name, const char *uri){}

    virtual void outputInt(__int64 field, unsigned size, const char *fieldName);
    virtual void outputUInt(unsigned __int64 field, unsigned size, const char *fieldName);

    void newline();
protected:
    StringBuffer out;
};

class thorhelper_decl CommonFieldProcessor : implements IFieldProcessor, public CInterface
{
    bool trim;
    StringBuffer &result;
public:
    IMPLEMENT_IINTERFACE;
    CommonFieldProcessor(StringBuffer &_result, bool _trim=false);
    virtual void processString(unsigned len, const char *value, const RtlFieldInfo * field);
    virtual void processBool(bool value, const RtlFieldInfo * field);
    virtual void processData(unsigned len, const void *value, const RtlFieldInfo * field);
    virtual void processInt(__int64 value, const RtlFieldInfo * field);
    virtual void processUInt(unsigned __int64 value, const RtlFieldInfo * field);
    virtual void processReal(double value, const RtlFieldInfo * field);
    virtual void processDecimal(const void *value, unsigned digits, unsigned precision, const RtlFieldInfo * field);
    virtual void processUDecimal(const void *value, unsigned digits, unsigned precision, const RtlFieldInfo * field);
    virtual void processUnicode(unsigned len, const UChar *value, const RtlFieldInfo * field);
    virtual void processQString(unsigned len, const char *value, const RtlFieldInfo * field);
    virtual void processUtf8(unsigned len, const char *value, const RtlFieldInfo * field);

    virtual bool processBeginSet(const RtlFieldInfo * field, unsigned numElements, bool isAll, const byte *data);
    virtual bool processBeginDataset(const RtlFieldInfo * field, unsigned numRows);
    virtual bool processBeginRow(const RtlFieldInfo * field);
    virtual void processEndSet(const RtlFieldInfo * field);
    virtual void processEndDataset(const RtlFieldInfo * field);
    virtual void processEndRow(const RtlFieldInfo * field);

};

extern thorhelper_decl void printKeyedValues(StringBuffer &out, IIndexReadContext *segs, IOutputMetaData *rowMeta);

extern thorhelper_decl void convertRowToXML(size32_t & lenResult, char * & result, IOutputMetaData & info, const void * row, unsigned flags = (unsigned)-1);
extern thorhelper_decl void convertRowToJSON(size32_t & lenResult, char * & result, IOutputMetaData & info, const void * row, unsigned flags = (unsigned)-1);

struct CSVOptions
{
    StringAttr delimiter, terminator;
    bool includeHeader;
};

class CCSVRow;

class CCSVItem : public CInterface, implements IInterface
{
    unsigned columnID, nextRowID, rowCount, nestedLayer;
    StringAttr name, type, value, parentXPath;
    StringArray childNames;
    MapStringTo<bool> headNameMap;
    bool isNestedItem, simpleNested, currentRowEmpty;
public:
    CCSVItem() : columnID(0), nestedLayer(0), nextRowID(0), rowCount(0), isNestedItem(false),
        simpleNested(false), currentRowEmpty(true) { };

    IMPLEMENT_IINTERFACE;
    inline const char* getName() const { return name.get(); };
    inline void setName(const char* _name) { name.set(_name); };
    inline const char* getValue() const { return value.get(); };
    inline void setValue(const char* _value) { value.set(_value); };
    inline const char* getType() const { return type.get(); };
    inline void setType(const char* _type) { type.set(_type); };
    inline unsigned getColumnID() const { return columnID; };
    inline void setColumnID(unsigned _columnID) { columnID = _columnID; };

    inline unsigned getNextRowID() const { return nextRowID; };
    inline void setNextRowID(unsigned _rowID) { nextRowID = _rowID; };
    inline void incrementNextRowID() { nextRowID++; };
    inline unsigned getRowCount() const { return rowCount; };
    inline void setRowCount(unsigned _rowCount) { rowCount = _rowCount; };
    inline void incrementRowCount() { rowCount++; };
    inline bool getCurrentRowEmpty() const { return currentRowEmpty; };
    inline void setCurrentRowEmpty(bool _currentRowEmpty) { currentRowEmpty = _currentRowEmpty; };

    inline bool checkIsNestedItem() const { return isNestedItem; };
    inline void setIsNestedItem(bool _isNestedItem) { isNestedItem = _isNestedItem; };
    inline bool checkSimpleNested() const { return simpleNested; };
    inline void setSimpleNested(bool _simpleNested) { simpleNested = _simpleNested; };
    inline const char* getParentXPath() const { return parentXPath.str(); };
    inline void setParentXPath(const char* _parentXPath) { parentXPath.set(_parentXPath); };
    inline unsigned getChildrenCount() { return childNames.length(); };
    inline StringArray& getChildrenNames() { return childNames; };
    inline void addChildName(const char* name)
    {
        if (hasChildName(name))
            return;
        headNameMap.setValue(name, true);
        childNames.append(name);
    };
    inline bool hasChildName(const char* name)
    {
        bool* found = headNameMap.getValue(name);
        return (found && *found);
    };
    inline unsigned getNestedLayer() const { return nestedLayer; };
    inline void setNestedLayer(unsigned _nestedLayer) { nestedLayer = _nestedLayer; };

    void clearContent()
    {
        nextRowID = rowCount = 0;
        currentRowEmpty = true;
    };
};

class CCSVRow : public CInterface, implements IInterface
{
    unsigned rowID;
    CIArrayOf<CCSVItem> columns;
public:
    CCSVRow(unsigned _rowID) : rowID(_rowID) {};
    IMPLEMENT_IINTERFACE;

    inline unsigned getColumnID() const { return rowID; };
    inline void setColumnID(unsigned _rowID) { rowID = _rowID; };
    inline unsigned getColumnCount() const { return columns.length(); };
    const char* getColumnValue(unsigned columnID) const
    {
        if (columnID >= columns.length())
            return "";
        CCSVItem& column = columns.item(columnID);
        return column.getValue();
    };
    void addColumn(unsigned columnID, const char* columnName, const char* columnValue)
    {
        unsigned len = columns.length();
        if (columnID < len)
        {
            CCSVItem& column = columns.item(columnID);
            if (columnName && *columnName)
                column.setName(columnName);
            column.setValue(columnValue);
        }
        else
        {
            for (unsigned i = len; i <= columnID; i++)
            {
                Owned<CCSVItem> column = new CCSVItem();
                if (i == columnID)
                {
                    if (columnName && *columnName)
                        column->setName(columnName);
                    column->setValue(columnValue);
                }
                columns.append(*column.getClear());
            }
        }
    };
};

#define ONE_RECORD_TEST
#define DEBUGHEADER
#define DEBUG_OUT

//CommonCSVWriter can be used to output a WU result in CSV format.
//Read CSV headers and store into the 'out' buffer;
//Read each row (a record) of the WU result and store into the 'out' buffer;
//The 'out' buffer can be accessed through the str() method.
static char hexchar[] = "0123456789ABCDEF";
static char csvQuote = '\"';

class thorhelper_decl CommonCSVWriter: public CInterface, implements IXmlWriterExt
{
    bool headerDone, addingSimpleNested;
    CSVOptions options;
    unsigned recordCount, rowCount, columnID, nestedHeaderLayerID, nestedContentLayerID;
    StringBuffer currentParentXPath;
    MapStringTo<bool> headNameMap;
    StringArray headerXPathList;
    MapStringToMyClass<CCSVItem> csvItems;
    CIArrayOf<CCSVRow> contentRowsBuffer;
#ifdef DEBUG_OUT
    StringBuffer debugOut;
#endif

    void escapeQuoted(unsigned len, char const* in, StringBuffer& out)
    {
        char const* finger = in;
        while (len--)
        {
            //RFC-4180, paragraph "If double-quotes are used to enclose fields, then a double-quote
            //appearing inside a field must be escaped by preceding it with another double quote."
            //unsigned newLen = 0;
            if (*finger == '"')
                out.append('"');
            out.append(*finger);
            finger++;
        }
    };
    bool foundHeader(const char* name)
    {
        if (currentParentXPath.isEmpty())
        {
            bool* found = headNameMap.getValue(name);
            return (found && *found);
        }
        CCSVItem* item = getParentCSVItem();
        if (!item)
            return false;

        addingSimpleNested = item->checkSimpleNested();
        if (addingSimpleNested) //ECL: SET OF string, int, etc
            return true;
        return item->hasChildName(name);
    };
    CCSVItem* getParentCSVItem()
    {
        StringBuffer path = currentParentXPath;
        path.setLength(path.length() - 1);
        return csvItems.getValue(path.str());
    };
    CCSVItem* getCSVItemByName(const char* name)
    {
        StringBuffer path;
        if (currentParentXPath.isEmpty())
            path.append(name);
        else
            path.append(currentParentXPath.str()).append(name);
        return csvItems.getValue(path);
    };
    unsigned getColumnID(const char* name)
    {
        CCSVItem* item = NULL;
        if (addingSimpleNested)
            item = getParentCSVItem();
        else //This should never happen.
            item = getCSVItemByName(name);
        if (!item)
            return 0;

        return item->getColumnID();
    };
    unsigned getNextRowID(const char* name)
    {
        CCSVItem* item = NULL;
        if (addingSimpleNested)
            item = getParentCSVItem();
        else
            item = getCSVItemByName(name);
        if (!item) //This should never happen.
            return 0;

        return item->getNextRowID();
    };
    void setColumn(CIArrayOf<CCSVRow>& rows, unsigned rowID, unsigned colID, const char* columnValue, const char* columnName)
    {
        if (!columnValue)
            columnValue = "";
        if (rowID < rows.length())
        {
            CCSVRow& row = rows.item(rowID);
            row.addColumn(colID, NULL, columnValue);
        }
        else
        { //new row
            Owned<CCSVRow> newRow = new CCSVRow(rowID);
            newRow->addColumn(colID, NULL, columnValue);
            rows.append(*newRow.getClear());
        }

        if (currentParentXPath.isEmpty())
            return;

        if (!addingSimpleNested)
        {
            if (columnName && *columnName)
            {
                CCSVItem* item = getCSVItemByName(columnName);
                if (item)
                    item->incrementNextRowID();
            }
        }

        CCSVItem* parentItem = getParentCSVItem();
        if (parentItem)
        {
            if (addingSimpleNested)
                parentItem->incrementNextRowID();
            parentItem->setCurrentRowEmpty(false);
        }
    };
    void addCSVHeader(const char* name, const char* type, bool isNested, bool simpleNested)
    {
        if (foundHeader(name))
            return;

        Owned<CCSVItem> headerItem = new CCSVItem();
        headerItem->setName(name);
        headerItem->setIsNestedItem(isNested);
        headerItem->setSimpleNested(simpleNested);
        if (type && *type)
            headerItem->setType(type);
        headerItem->setColumnID(columnID);
        headerItem->setNestedLayer(nestedHeaderLayerID);
        headerItem->setParentXPath(currentParentXPath.str());
        StringBuffer xPath = currentParentXPath;
        xPath.append(name);
        csvItems.setValue(xPath.str(), headerItem);

        headerXPathList.append(xPath.str());
        addChildNameToParentCSVItem(name);
        if (currentParentXPath.isEmpty())
            headNameMap.setValue(name, true);
    };
    void addContentField(const char* field, const char* fieldName)
    {
        setColumn(contentRowsBuffer, getNextRowID(fieldName), getColumnID(fieldName), field, fieldName);
#ifdef DEBUG_OUT
        debugOut.appendf("addContentField: parent<%s> row<%d> col<%d> - name<%s> value<%s>\n",
            currentParentXPath.str(), getNextRowID(fieldName), getColumnID(fieldName), fieldName, field);
#endif
    };
    void addStringField(unsigned len, const char* field, const char* fieldName)
    {
        StringBuffer v;
        v.append(csvQuote);
        escapeQuoted(len, field, v);
        v.append(csvQuote);
        addContentField(v.str(), fieldName);
    };
    unsigned getMaxChildrenNextRowID(const char* path)
    {
        CCSVItem* item = csvItems.getValue(path);
        if (!item->checkIsNestedItem())
            return item->getNextRowID();

        unsigned maxRowID = 0;
        StringBuffer basePath = path;
        basePath.append("/");
        StringArray& names = item->getChildrenNames();
        ForEachItemIn(i, names)
        {
            StringBuffer childPath = basePath;
            childPath.append(names.item(i));
            unsigned rowID = getMaxChildrenNextRowID(childPath.str());
            if (rowID > maxRowID)
                maxRowID = rowID;
        }
        return maxRowID;
    };
    void setChildrenNextRowID(const char* path, unsigned rowID)
    {
        CCSVItem* item = csvItems.getValue(path);
        if (!item)
            return;

        if (!item->checkIsNestedItem())
        {
            item->setNextRowID(rowID);
            return;
        }

        StringArray& names = item->getChildrenNames();
        ForEachItemIn(i, names)
        {
            StringBuffer childPath = path;
            childPath.append("/").append(names.item(i));

            CCSVItem* childItem = csvItems.getValue(childPath.str());
            childItem->setNextRowID(rowID);//for possible new row
#ifdef DEBUG_OUT
            debugOut.appendf("setChildrenNextRowID path<%s> - rowID<%d>\n", childPath.str(), rowID);
#endif
            if (childItem->checkIsNestedItem())
            {
                childItem->setRowCount(0);
                setChildrenNextRowID(childPath.str(), rowID);
            }
        }
    };
    void addChildNameToParentCSVItem(const char* name)
    {
        if (!name || !*name)
            return;

        if (currentParentXPath.isEmpty())
            return;

        CCSVItem* item = getParentCSVItem();
        if (item)
            item->addChildName(name);
    };
    void removeFieldFromCurrentParentXPath(const char* fieldName)
    {
        unsigned len = strlen(fieldName);
        if (currentParentXPath.length() > len+1)
            currentParentXPath.setLength(currentParentXPath.length() - len - 1);
        else
            currentParentXPath.setLength(0);
    };
    void setOutputBuffer(CIArrayOf<CCSVRow>& rows, bool isHeader)
    {
        bool firstRow = true;
        ForEachItemIn(i, rows)
        {
            if (firstRow && !isHeader)
            {
                out.append(recordCount);
                firstRow = false;
            }

            CCSVRow& row = rows.item(i);
            unsigned len = row.getColumnCount();
            for (unsigned col = 0; col < len; col++)
                out.append(options.delimiter.get()).append(row.getColumnValue(col));
            out.append(options.terminator.get());
        }
    };
    void endRecord()
    {
        recordCount++;
        setOutputBuffer(contentRowsBuffer, false);

        //Prepare for possible next record
        currentParentXPath.setLength(0);
        contentRowsBuffer.kill();
        ForEachItemIn(i, headerXPathList)
        {
            const char* path = headerXPathList.item(i);
            CCSVItem* item = csvItems.getValue(path);
            if (item)
                item->clearContent();
        }
    };
#ifdef DEBUG_OUT
    void dumpHeaderInfo()
    {
        return;
        ForEachItemIn(i, headerXPathList)
        {
            const char* path = headerXPathList.item(i);
            CCSVItem* item = csvItems.getValue(path);
            if (!item)
                continue;
            if (!item->checkIsNestedItem())
            {
                debugOut.appendf("dumpHeaderInfo path<%s> next row<%d> col<%d>: name<%s> - value<%s>\n", path, item->getNextRowID(),
                    item->getColumnID(), item->getName() ? item->getName() : "", item->getValue() ? item->getValue() : "");
            }
            else
            {
                debugOut.appendf("dumpHeaderInfo path<%s> next row<%d> col<%d>: name<%s> - value<%s>\n", path, item->getNextRowID(),
                    item->getColumnID(), item->getName() ? item->getName() : "", item->getValue() ? item->getValue() : "");
            }
        }
    }
#endif

public:
    CommonCSVWriter(unsigned _flags, CSVOptions& _options, IXmlStreamFlusher* _flusher = NULL);
    ~CommonCSVWriter();

    IMPLEMENT_IINTERFACE;

    inline void flush(bool isClose)
    {
        if (flusher)
            flusher->flushXML(out, isClose);
    }

#ifdef DEBUG_OUT
    const char* debugStr() const { return debugOut.str(); }
#endif
    virtual unsigned length() const { return out.length(); }
    virtual const char* str() const { return out.str(); }
    virtual IInterface* saveLocation() const
    {
        if (flusher)
            throwUnexpected();
        return NULL;
    };
    virtual void rewindTo(IInterface* location)
    {
        //Unimplemented;
    };

    //IXmlWriter
    virtual void outputQuoted(const char* text)
    {
        //No fieldName. Is it valid for CSV?
    };
    virtual void outputString(unsigned len, const char* field, const char* fieldName)
    {
#ifdef ONE_RECORD_TEST
        if (recordCount > 1)
            return;
#endif
        if (!foundHeader(fieldName))
            return;
        addStringField(len, field, fieldName);
    };
    virtual void outputBool(bool field, const char* fieldName)
    {
#ifdef ONE_RECORD_TEST
        if (recordCount > 1)
            return;
#endif
        if (!foundHeader(fieldName))
            return;
        addContentField((field) ? "true" : "false", fieldName);
    };
    virtual void outputData(unsigned len, const void* field, const char* fieldName)
    {
#ifdef ONE_RECORD_TEST
        if (recordCount > 1)
            return;
#endif
        if (!foundHeader(fieldName))
            return;

        StringBuffer v;
        const unsigned char *value = (const unsigned char *) field;
        for (unsigned int i = 0; i < len; i++)
            v.append(hexchar[value[i] >> 4]).append(hexchar[value[i] & 0x0f]);
        addContentField(v.str(), fieldName);
    };
    virtual void outputInt(__int64 field, unsigned size, const char* fieldName)
    {
#ifdef ONE_RECORD_TEST
        if (recordCount > 1)
            return;
#endif
        if (!foundHeader(fieldName))
            return;

        StringBuffer v;
        v.append(field);
        addContentField(v.str(), fieldName);
    };
    virtual void outputUInt(unsigned __int64 field, unsigned size, const char* fieldName)
    {
#ifdef ONE_RECORD_TEST
        if (recordCount > 1)
            return;
#endif
        if (!foundHeader(fieldName))
            return;

        StringBuffer v;
        v.append(field);
        addContentField(v.str(), fieldName);
    };
    virtual void outputReal(double field, const char *fieldName)
    {
#ifdef ONE_RECORD_TEST
        if (recordCount > 1)
            return;
#endif
        if (!foundHeader(fieldName))
            return;

        StringBuffer v;
        v.append(field);
        addContentField(v.str(), fieldName);
    };
    virtual void outputDecimal(const void* field, unsigned size, unsigned precision, const char* fieldName);
    virtual void outputUDecimal(const void* field, unsigned size, unsigned precision, const char* fieldName);
    virtual void outputUnicode(unsigned len, const UChar* field, const char* fieldName);
    virtual void outputQString(unsigned len, const char* field, const char* fieldName);
    virtual void outputBeginDataset(const char* dsname, bool nestChildren)
    {
        return;
    };
    virtual void outputEndDataset(const char* dsname)
    {
        return;
    };
    virtual void outputBeginNested(const char* fieldName, bool simpleNested)
    {
        if (!headerDone)
        {
            addCSVHeader(fieldName, NULL, true, simpleNested);
            if (simpleNested)
                columnID++;
            nestedHeaderLayerID++;
            currentParentXPath.append(fieldName).append("/");
            return;
        }
#ifdef ONE_RECORD_TEST
        if (recordCount > 1)
            return;
#endif

#ifdef DEBUG_OUT
        debugOut.appendf("outputBeginNested: enter layer<%d> parent<%s> - name<%s> \n", nestedContentLayerID, currentParentXPath.str(), fieldName);
#endif
        if (!strieq(fieldName, "row"))
        {//Find out a nested item
            nestedContentLayerID++;
            currentParentXPath.append(fieldName).append("/");
        }
        else if (!currentParentXPath.isEmpty())
        {//Begin a new row inside the nested item
            CCSVItem* item = getParentCSVItem();
            if (!item)
                return;

            unsigned rowCount = item->getRowCount();
            if (rowCount > 0)
            {
                StringBuffer path = currentParentXPath;
                path.setLength(path.length() - 1);
                setChildrenNextRowID(path.str(), getMaxChildrenNextRowID(path.str()));
            }

            item->setCurrentRowEmpty(true);
        }
    };
    virtual void outputEndNested(const char* fieldName)
    {
        if (!fieldName || !*fieldName)
            return;

        if (!headerDone)
        {
            nestedHeaderLayerID--;
            removeFieldFromCurrentParentXPath(fieldName);
            return;
        }

#ifdef ONE_RECORD_TEST
        if (recordCount > 1)
            return;
#endif
#ifdef DEBUG_OUT
        debugOut.appendf("outputEndNested: leave layer<%d>, parent<%s> - name<%s> \n", nestedContentLayerID, currentParentXPath.str(), fieldName);
#endif
        if (strieq(fieldName, "row"))
        {//End a row inside the nested item
            if (!currentParentXPath.isEmpty())
            {
                CCSVItem* item = getParentCSVItem();
                if (item && !item->getCurrentRowEmpty())
                {
                    item->incrementRowCount();
                    item->setCurrentRowEmpty(true);
                }
            }
            if (nestedContentLayerID == 0) //outputEndNested Row at layer 0 indicates the end of a WU Result row.
                endRecord();
        }
        else
        {//This is an end of a nested item.
            removeFieldFromCurrentParentXPath(fieldName);
            nestedContentLayerID--;
        }
    };
    virtual void outputBeginArray(const char* fieldName)
    {
#ifdef ONE_RECORD_TEST
        if (recordCount > 1)
            return;
#endif
#ifdef DEBUG_OUT
        debugOut.appendf("outputBeginArray: name<%s> \n", fieldName);
#endif
    };
    virtual void outputEndArray(const char* fieldName)
    {
#ifdef ONE_RECORD_TEST
        if (recordCount > 1)
            return;
#endif
#ifdef DEBUG_OUT
        debugOut.appendf("outputEndArray: name<%s> \n", fieldName);
#endif
    };
    virtual void outputSetAll()
    {
        return;
    };
    virtual void outputUtf8(unsigned len, const char* field, const char* fieldName);
    virtual void outputInlineXml(const char* text)//for appending raw xml content
    {
        //Dynamically add a new header 'xml' and insert the header.
        //But, not sure we want to do that for a big WU result.
        //if (text && *text)
          //outputUtf8(strlen(text), text, "xml");
    };
    virtual void outputXmlns(const char* name, const char* uri)
    {
        return;
    };

    //IXmlWriterExt
    virtual void outputNumericString(const char* field, const char* fieldName)
    {
#ifdef ONE_RECORD_TEST
        if (recordCount > 1)
            return;
#endif
        if (!foundHeader(fieldName))
            return;

        addStringField((size32_t)strlen(field), field, fieldName);
    };
    virtual IXmlWriterExt& clear()
    {
        out.clear();
        return *this;
    };

    void outputCSVHeader(const char* name, const char* type)
    {
        if (!name || !*name)
            return;
        addCSVHeader(name, type, false, false);
        columnID++;
    };
    void outputHeadersToBuffer()
    {
        CIArrayOf<CCSVRow> rows;
        ForEachItemIn(i, headerXPathList)
        {
            const char* path = headerXPathList.item(i);
            CCSVItem* item = csvItems.getValue(path);
            if (!item)
                continue;

            unsigned colID = item->getColumnID();
            if (item->checkIsNestedItem())
            {
                unsigned childCount = item->getChildrenCount();
                if (childCount > 1)
                    childCount--;
                colID += childCount/2;
            }
            setColumn(rows, item->getNestedLayer(), colID, item->getName(), NULL);
        }

        setOutputBuffer(rows, true);
    };
    void finishCSVHeaders()
    {
        headerDone = true;
        rowCount = 0;
        currentParentXPath.clear();
    };
protected:
    IXmlStreamFlusher* flusher;
    StringBuffer out;
    unsigned flags;
};

#endif // THORXMLWRITE_HPP
