/*
Copyright (C) Krzysztof Kowalczyk
Owner: Krzysztof Kowalczyk (krzysztofk@pobox.com)
*/


#include "PrefsStore.hpp"

#include <Debug.hpp>
#include <iPediaApplication.hpp>
#include <aygshell.h>
#ifndef PPC
#include <tpcshell.h>
#include <winuserm.h>
#endif
#include <shellapi.h>
#include <winbase.h>
#include <sms.h>

#define Assert assert

#ifdef new
#undef new
#endif

#ifndef NDEBUG
#define new_malloc(size) ::operator new ((size), __FILE__, __LINE__)
#else
#define new_malloc(size) ::operator new ((size))
#endif // NDEBUG

#define new_free(ptr) ::operator delete ((ptr))

//! @todo implement IsValidPrefRecord()
#define IsValidPrefRecord(recData) true

#ifndef PPC
    #define STORE_FOLDER CSIDL_APPDATA
#else
    #define STORE_FOLDER CSIDL_PROGRAMS
#endif

namespace ArsLexis 
{
    int getPrefItemSize(PrefItem& item)
    {
        switch(item.type)
        {
            case pitBool:
                return sizeof(bool);
            case pitInt:
                return sizeof(int);
            case pitLong:
                return sizeof(long);
            case pitUInt16:
                return sizeof(ushort_t);
            case pitUInt32:
                return sizeof(ulong_t);
            case pitStr:
                return tstrlen(item.value.strVal)*2;
        }       
        return 0;
    }

    
/*
The idea is to provide an API for storing/reading preferences
in a Palm database in a format that is easily upgradeable.
Each item in a preferences database has a type (int, bool, string)
and unique id. That way, when we add new items we want to store in
preferences database we just create a new unique id. The saving part
just needs to be updated to save a new item, reading part must be updated
to ignore the case when a preference item is missing.

 We provide separate classes for reading and writing preferences
 because they're used in that way.
 
  To save preferences:
  * construct PrefsStoreWriter object
  * call ErrSet*() to set all preferences items
  * call ErrSavePreferences()
  
   To read preferences:
   * construct PrefsStoreReader object
   * call ErrGet*() to get desired preferences items
   
    Serialization format:
    * data is stored as blob in one record in a database whose name, creator and
    type are provided by the caller
    * first 4-bytes of a blob is a header (to provide some protection against
    reading stuff we didn't create)
    * then we have each item serialized
    
     Serialization of an item:
     * 2-byte unique id
     * 2-byte type of an item (bool, int, string)
     * bool is 1 byte (0/1)
     * ushort_t is 2-byte unsigned int
     * ulong_t is 4-byte unsigned int
     * string is: 4-byte length of the string (including terminating 0) followed
     by string characters (also including terminating 0)
    */
    
    
    PrefsStoreReader::PrefsStoreReader(const char_t *dbName, ulong_t dbCreator, ulong_t dbType)
        : _dbName(dbName), _dbCreator(dbCreator), _dbType(dbType),_fHandle(INVALID_HANDLE_VALUE)
    {
        TCHAR szPath[MAX_PATH];
        BOOL f = SHGetSpecialFolderPath(iPediaApplication::instance().getMainWindow(), szPath, STORE_FOLDER, FALSE);
        char_t *myDBName = new char_t[tstrlen(dbName)+1];
        wcscpy(myDBName, dbName);
        String fullPath;
        fullPath.assign(szPath);
        fullPath.append(_T("\\"));
        for(unsigned int i=0; i<tstrlen(myDBName);i++)
            if(myDBName[i]==' ') 
                myDBName[i]='_';
        fullPath.append(myDBName);
        delete myDBName;
        //fullPath+=fileName;
        _fHandle = CreateFile(fullPath.c_str(), 
            GENERIC_READ, FILE_SHARE_READ, NULL, 
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 
            NULL); 
    }
    
    PrefsStoreReader::~PrefsStoreReader()
    {
        /*for (std::map< int, PrefItem>::iterator it=items_.begin();
            it!=items_.end();it++)
            if(it->second->type==pitStr)
                delete it->second.value.strVal[];*/
        while(!strPointers_.empty())
        {
            delete [] strPointers_.front();
            strPointers_.pop_front();
        }
        //strPointers_.push_back(prefItem.value.strVal);
        CloseHandle(_fHandle);

    }
    
#define PREFS_STORE_RECORD_ID "aRSp"  // comes from "ArsLexis preferences"
#define FValidPrefsStoreRecord(recData) (0==MemCmp(recData,PREFS_STORE_RECORD_ID,StrLen(PREFS_STORE_RECORD_ID)))
    
  
    
    status_t PrefsStoreReader::ErrGetBool(int uniqueId, bool *value)
    {
        PrefItem    prefItem;
        status_t err = ErrGetPrefItemWithId(uniqueId, &prefItem);
        if (err)
            return err;
        Assert(prefItem.uniqueId == uniqueId);
        if (prefItem.type != pitBool)
            return psErrItemTypeMismatch;
        *value = prefItem.value.boolVal;
        return errNone;
    }
    
    status_t PrefsStoreReader::ErrGetInt(int uniqueId, int *value)
    {
        PrefItem    prefItem;
        status_t err = ErrGetPrefItemWithId(uniqueId, &prefItem);
        if (err)
            return err;
        assert(prefItem.uniqueId == uniqueId);
        if (prefItem.type != pitInt)
            return psErrItemTypeMismatch;
        *value = prefItem.value.intVal;
        return errNone;
    }
    
    status_t PrefsStoreReader::ErrGetLong(int uniqueId, long *value)
    {
        PrefItem    prefItem;
        status_t err = ErrGetPrefItemWithId(uniqueId, &prefItem);
        if (err)
            return err;
        Assert(prefItem.uniqueId == uniqueId);
        if (prefItem.type != pitLong)
            return psErrItemTypeMismatch;
        *value = prefItem.value.longVal;
        return errNone;
    }
    
    status_t PrefsStoreReader::ErrGetUInt16(int uniqueId, ushort_t *value)
    {
        PrefItem    prefItem;
        status_t err = ErrGetPrefItemWithId(uniqueId, &prefItem);
        if (err)
            return err;
        assert(prefItem.uniqueId == uniqueId);
        if (prefItem.type != pitUInt16)
            return psErrItemTypeMismatch;
        *value = prefItem.value.uint16Val;
        return errNone;
    }
    
    status_t PrefsStoreReader::ErrGetUInt32(int uniqueId, ulong_t *value)
    {
        PrefItem    prefItem;
        status_t err = ErrGetPrefItemWithId(uniqueId, &prefItem);
        if (err)
            return err;
        assert(prefItem.uniqueId == uniqueId);
        if (prefItem.type != pitUInt32)
            return psErrItemTypeMismatch;
        *value = prefItem.value.uint32Val;
        return errNone;
    }
    
    // the string returned points to data inside the object that is only valid
    // while the object is alive. If client wants to use it after that, he must
    // make a copy
    status_t PrefsStoreReader::ErrGetStr(int uniqueId, const char_t **value)
    {
        PrefItem    prefItem;
        status_t err = ErrGetPrefItemWithId(uniqueId, &prefItem);
        if (err)
            return err;
        assert(prefItem.uniqueId == uniqueId);
        if (prefItem.type != pitStr)
            return psErrItemTypeMismatch;
        *value = prefItem.value.strVal;
        return errNone;
    }

    status_t PrefsStoreReader::ErrLoadPreferences()
    {
        if(_fHandle==INVALID_HANDLE_VALUE)
            return psErrNoPrefDatabase;
        PrefItem prefItem;
        ulong_t read=0;
        int size=0;
        do
        {
            ReadFile(_fHandle, &prefItem.uniqueId, sizeof(prefItem.uniqueId),&read, NULL);
            if(read==0) 
                return errNone;
            if(read!=sizeof(prefItem.uniqueId)) 
                return psErrDatabaseCorrupted;
            ReadFile(_fHandle, &size, sizeof(size),&read, NULL);
            if(read!=sizeof(size)) 
                return psErrDatabaseCorrupted;
            ReadFile(_fHandle, &prefItem.type, sizeof(prefItem.type),&read, NULL);
            if(read!=sizeof(prefItem.type)) 
                return psErrDatabaseCorrupted;
            if(prefItem.type != pitStr)
            {
                ReadFile(_fHandle, &prefItem.value, size,&read, NULL);
                if(read!=size) 
                    return psErrDatabaseCorrupted;
            }
            else
            {
                char_t *str = new char_t[size/2+1];
                prefItem.value.strVal = str;
                strPointers_.push_back(str);
                ReadFile(_fHandle, (void*)prefItem.value.strVal, size,&read, NULL);
                if(read!=size) 
                    return psErrDatabaseCorrupted;
            }
            items_.insert(std::pair<int, PrefItem>(prefItem.uniqueId,prefItem));
        }
        while(true);
        return errNone;
    }

    status_t PrefsStoreReader::ErrGetPrefItemWithId(int uniqueId, PrefItem *prefItem)
    {
        status_t err = errNone;
        if(items_.empty())        
        {
            err = ErrLoadPreferences();
            if(err!=errNone)
                return err;
        }
        std::map< int, PrefItem>::iterator it=items_.find(uniqueId);
        if(it==items_.end())
            return psErrItemNotFound;
        *prefItem=it->second;
        //Direct reading from file
        return errNone;
    }
    
    PrefsStoreWriter::PrefsStoreWriter(const char_t *dbName, ulong_t dbCreator, ulong_t dbType)
        : _dbName(dbName), _dbCreator(dbCreator), _dbType(dbType), _itemsCount(0),_fHandle(INVALID_HANDLE_VALUE)
    {
        TCHAR szPath[MAX_PATH];
        BOOL f = SHGetSpecialFolderPath(iPediaApplication::instance().getMainWindow(), szPath, STORE_FOLDER, FALSE);
        char_t *myDBName = new char_t[tstrlen(dbName)+1];
        wcscpy(myDBName, dbName);
        String fullPath;
        fullPath.assign(szPath);
        fullPath.append(_T("\\"));
        for(unsigned int i=0; i<tstrlen(myDBName);i++)
            if(myDBName[i]==' ') 
                myDBName[i]='_';
        fullPath.append(myDBName);
        delete myDBName;
        //CreateDirectory (fullPath.c_str(), NULL);    
        //fullPath+=fileName;
        _fHandle = CreateFile(fullPath.c_str(), 
            GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL); 
    }
    
    PrefsStoreWriter::~PrefsStoreWriter()
    {
        if(_fHandle!=INVALID_HANDLE_VALUE)
            CloseHandle(_fHandle);
    }
            
    status_t PrefsStoreWriter::ErrSetBool(int uniqueId, bool value)
    {
        PrefItem    prefItem;
    
        prefItem.type = pitBool;
        prefItem.uniqueId = uniqueId;
        prefItem.value.boolVal = value;
        
        return ErrSetItem( &prefItem );
    }
    
    status_t PrefsStoreWriter::ErrSetInt(int uniqueId, int value)
    {
        PrefItem    prefItem;
    
        prefItem.type = pitInt;
        prefItem.uniqueId = uniqueId;
        prefItem.value.intVal = value;
     
        return ErrSetItem( &prefItem );
    }
    
    status_t PrefsStoreWriter::ErrSetLong(int uniqueId, long value)
    {
        PrefItem    prefItem;
    
        prefItem.type = pitLong;
        prefItem.uniqueId = uniqueId;
        prefItem.value.longVal = value;
     
        return ErrSetItem( &prefItem );
    }
    
    status_t PrefsStoreWriter::ErrSetUInt16(int uniqueId, ushort_t value)
    {
        PrefItem    prefItem;
    
        prefItem.type = pitUInt16;
        prefItem.uniqueId = uniqueId;
        prefItem.value.uint16Val = value;
     
        return ErrSetItem( &prefItem );
    }
    
    status_t PrefsStoreWriter::ErrSetUInt32(int uniqueId, ulong_t value)
    {
        PrefItem    prefItem;
    
        prefItem.type = pitUInt32;
        prefItem.uniqueId = uniqueId;
        prefItem.value.uint32Val = value;
    
        return ErrSetItem( &prefItem );        
    }
    
    // value must point to a valid location during ErrSavePrefernces() since
    // for perf reasons we don't make a copy of the string
    status_t PrefsStoreWriter::ErrSetStr(int uniqueId, const char_t *value)
    {
        PrefItem    prefItem;
    
        prefItem.type = pitStr;
        prefItem.uniqueId = uniqueId;
        prefItem.value.strVal = value;
     
        return ErrSetItem( &prefItem );
    }
    
    status_t PrefsStoreWriter::ErrSetItem(PrefItem *item)
    {
        if(items_.find(item->uniqueId)!=items_.end())
            return psErrDuplicateId;
        
        items_.insert(std::pair<int, PrefItem>(item->uniqueId,*item));
        return errNone;
    }
    
    status_t PrefsStoreWriter::ErrSavePreferences()
    {
        if(_fHandle==INVALID_HANDLE_VALUE)
            return psErrNoPrefDatabase;
        DWORD written;
        for (std::map< int, PrefItem>::iterator it=items_.begin();
            it!=items_.end();it++)
        {
            PrefItem prefItem=it->second;
            int size=getPrefItemSize(prefItem);
            WriteFile(_fHandle, &prefItem.uniqueId, sizeof(prefItem.uniqueId),&written, NULL);
            WriteFile(_fHandle, &size, sizeof(size),&written, NULL);
            WriteFile(_fHandle, &prefItem.type, sizeof(int),&written, NULL);
            if(prefItem.type != pitStr)
                WriteFile(_fHandle, &prefItem.value, size,&written, NULL);
            else
                WriteFile(_fHandle, prefItem.value.strVal, size,&written, NULL);
        }
        return errNone;
    }
} // namespace ArsLexis
