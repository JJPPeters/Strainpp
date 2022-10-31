//
// Created by Jon on 10/05/2015.
//

#ifndef READDM_TAGREADER_H
#define READDM_TAGREADER_H

#include <iostream>

#include "dmutils.h"
#include "streamreader.h"


//TODO: would it be sensible to detect large array lengths? Other examples seem to do this but it seems a bit bodgy to me.
//TODO: add proper error handling.
//TODO: test everything.
//TODO: revert to boost::variant

namespace DMRead
{

    typedef std::tuple<int64_t, int64_t, int64_t> TSL; // I can't be bothered to write this

    // only way I could work out how to do this...
    static StreamReader Reader;

    enum TypeList {
        int16 = 2,
        int32 = 3,
        uint16 = 4,
        uint32 = 5,
        float32 = 6,
        float64 = 7,
        boolean32 = 8,
        int8 = 9,
        uint8 = 10,
        int64 = 11,
        uint64 = 12,
        structure = 15,
        string = 18,
        array = 20
    };

    template<class T>
    class TagReader {
    public:

        TagReader(std::map<std::string, TSL>& RefTagData);

    private:
        TypeList enumInst;

        std::map<std::string, TSL> TagData;

        int CurrentLevel;

        std::string CurrentTagName;

        bool SwapEndian;

        void StartTraverse();

        void GetTagEntry(int index); // accepts index as it is used a backup if no tag name

        void GetTagType();

        void AddData(T encType);

        void GetData(T &encType, int64_t &dataSize, int64_t &dataLocation);

        void AddTag(std::string name)
        {
            ++CurrentLevel;
            CurrentTagName +=  "." + name;
        }

        void RemoveTag()
        {
            --CurrentLevel;
            CurrentTagName = Utils::RemoveTagName(CurrentTagName);
        }
    };


    template<class T>
    TagReader<T>::TagReader(std::map<std::string, TSL>& RefTagData)
    {
        CurrentLevel = 0;
        CurrentTagName = "root";
        // Get file size in bytes (not needed here)
        //T fileSize = Reader.ReadNumeric<T>();
        Reader.Skip<T>();
        // test endian-ness
        uint32_t dle = Reader.ReadNumeric<uint32_t>();
        SwapEndian = (dle == 0) && Utils::TestEndian();
        // start reading
        StartTraverse();

        RefTagData = TagData;
    }

    template<typename T>
    void TagReader<T>::StartTraverse()
    {
        // is the group sorted (not needed here)
        //int isSorted = Reader.ReadStream<char>();
        Reader.Skip<char>();

        // is the group open? (not needed here)
        //int isOpen = Reader.ReadStream<char>();
        Reader.Skip<char>();

        // Get number of Tags and loop through them
        T numTags = Reader.ReadNumeric<T>();
        for (uint32_t i = 0; i < numTags; i++)
        {
            GetTagEntry(i);
        }
    }

    template<typename T>
    void TagReader<T>::GetTagEntry(int index)
    {
        int isData = Reader.ReadStream<char>();
        uint16_t labelSize = Reader.ReadNumeric<uint16_t>();
        std::string tagName = Reader.ReadString(labelSize);

        if (tagName.compare("") == 0)
            tagName = Utils::to_string(index); //TODO: Replace the patched version of to_string with std version (MinGW problem?)

        AddTag(tagName);

        if(sizeof(T)==8) //TODO: replace with member that holds the version number
        {
            // unused
            // T totalBytes = Reader.ReadNumeric<T>();
            Reader.Skip<T>();
        }

        if (isData == 21)
        {
            GetTagType();
        }
        else if (isData == 20)
        {
            StartTraverse();
        }
        else
        {
            //error handling
        }

        RemoveTag();
    }

    template <typename T>
    void TagReader<T>::GetTagType()
    {
        uint32_t delim = Reader.ReadNumeric<uint32_t>();
        if (delim != 623191333)// expect a group delimiter
        {
            throw std::runtime_error("DMReader: Encountered incorrect delimiter.");
        }
        //T defLen = Reader.ReadNumeric<T>(); // no real idea about this one too
        Reader.Skip<T>();
        T encType = Reader.ReadNumeric<T>(); // this tells us what data type the tag contains
        AddData(encType);
    }

    template <typename T>
    void TagReader<T>::AddData(T encType)
    {
        int64_t sz, loc;
        GetData(encType, sz, loc);
        TSL newEntry = std::make_tuple(encType, sz, loc);
        TagData[CurrentTagName] = newEntry;
    }

    template <typename T>
    void TagReader<T>::GetData(T &encType, int64_t &dataSize, int64_t &dataLocation)
    {

        SwapEndian = false;
        dataSize = 1;
        dataLocation = -1;

        switch (encType)
        {
            case int16:
                dataLocation = Reader.ReadNumericPos<int16_t>();
                break;
            case int32:
                dataLocation = Reader.ReadNumericPos<int32_t>();
                break;
            case uint16:
                dataLocation = Reader.ReadNumericPos<uint16_t>();
                break;
            case uint32:
                dataLocation = Reader.ReadNumericPos<uint32_t>();
                break;
            case float32:
                dataLocation = Reader.ReadNumericPos<float>();
                break;
            case float64:
                dataLocation = Reader.ReadNumericPos<double>();
                break;
            case int64:
                dataLocation = Reader.ReadNumericPos<int64_t>();
                break;
            case uint64:
                dataLocation = Reader.ReadNumericPos<uint64_t>();
                break;
            case int8:
                dataLocation = Reader.ReadNumericPos<char>();
                break;
            case uint8:
                dataLocation = Reader.ReadNumericPos<char>();
                break;
            case boolean32:
                dataLocation = Reader.ReadNumericPos<char>();
                break;
            case string:
                {
                    dataSize = Reader.ReadNumeric<uint32_t>();
                    dataLocation = Reader.ReadStringPos(dataSize);
                }
                break;
            case structure:
            {
                T nameLength = Reader.ReadNumeric<T>();
                T numFields = Reader.ReadNumeric<T>();
                std::vector<T> fieldnameLength(numFields);
                std::vector<T> fieldType(numFields);
                for (uint32_t i = 0; i < numFields; i++)
                {
                    fieldnameLength[i] = Reader.ReadNumeric<T>();
                    fieldType[i] = Reader.ReadNumeric<T>();
                }

                std::string structName = Reader.ReadString(nameLength);

                for(uint32_t i=0; i<numFields; i++)
                {
                    std::string fieldName = Reader.ReadString(fieldnameLength[i]);
                    if (fieldName == "")
                        fieldName = Utils::to_string(i);
                    if (structName != "")
                        fieldName = structName + "_" + fieldName;
                    AddTag(fieldName);

                    AddData(fieldType[i]);

                    RemoveTag();
                }

                RemoveTag();
            }
                break;
            case array:
            {
                T arrayType = Reader.ReadNumeric<T>();

                if (arrayType == structure)
                {
                    T nameLength = Reader.ReadNumeric<T>();
                    T numFields = Reader.ReadNumeric<T>();

                    std::vector<T> fieldnameLength(numFields);
                    std::vector<T> fieldType(numFields);
                    for (uint32_t i=0;i<numFields;i++)
                    {
                        fieldnameLength[i] = Reader.ReadNumeric<T>();
                        fieldType[i] = Reader.ReadNumeric<T>();
                    }

                    std::string structName = Reader.ReadString(nameLength);

                    T arrayLength = Reader.ReadNumeric<T>();

                    for(uint32_t j=0; j < arrayLength; j++)
                    {
                        for(uint32_t i=0; i<numFields; i++)
                        {
                            std::string fieldName = Reader.ReadString(fieldnameLength[i]);
                            if (fieldName == "")
                                fieldName = Utils::to_string(i);
                            if (structName != "")
                                fieldName = structName + "_" + fieldName;
                            AddTag(fieldName);

                            AddData(fieldType[i]);

                            RemoveTag();
                        }
                    }

                    RemoveTag();
                }
                else
                {
                    T arrayLength = Reader.ReadNumeric<T>();
                    dataSize = (uint64_t)arrayLength;

                    dataLocation = Reader.ReadPos();
                    encType = arrayType;
                    int64_t temp1, temp2;

                    // there has to be a better way
                    int bytesPer = 0;
                    switch (encType)
                    {
                        case int16:
                            bytesPer = 2;
                            break;
                        case int32:
                            bytesPer = 4;
                            break;
                        case uint16:
                            bytesPer = 2;
                            break;
                        case uint32:
                            bytesPer = 4;
                            break;
                        case float32:
                            bytesPer = 4;
                            break;
                        case float64:
                            bytesPer = 8;
                            break;
                        case int64:
                            bytesPer = 8;
                            break;
                        case uint64:
                            bytesPer = 8;
                            break;
                        case int8:
                            bytesPer = 1;
                            break;
                        case uint8:
                            bytesPer = 1;
                            break;
                        case boolean32:
                            bytesPer = 4;
                            break;
                        default:
                            for (uint32_t i=0; i<arrayLength; i++)
                                GetData(arrayType, temp1, temp2);
                            break;
                    }

                    Reader.Skip(bytesPer*arrayLength);
                    //for (uint32_t i=0; i<arrayLength; i++)
                    //    GetData(arrayType, temp1, temp2);
                }
                // assumed we can't have arrays of arrays (maybe we can?)
            }
                break;
            default:
                throw std::runtime_error("DMReader: Encountered unknown data type.");
                break;
        }

        SwapEndian = true;

        return;
    }
}


#endif //READDM_TAGREADER_H
