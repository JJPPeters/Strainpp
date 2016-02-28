//
// Created by Jon on 10/05/2015.
//

#ifndef READDM_STREAMREADER_H
#define READDM_STREAMREADER_H

#include <memory>
#include <cstring>
#include <cstdio>

namespace DMRead
{
    class StreamReader
    {
    public:
        FILE* filePtr;

        ~StreamReader()
        {
            closeStream();
        }

        void setStream(std::string filePath)
        {
            filePtr = fopen(filePath.c_str(), "rb");
            if(!filePtr)
                throw std::runtime_error("DMReader: Cannot open file.");
        }

        void closeStream()
        {
            fclose(filePtr);
        }

        template<typename T>
        T ReadStream()
        {
            T value;
            int size = sizeof(T);
            fread(&value, size*sizeof(char), 1, filePtr);

            return value;
        }

        template<typename T>
        T swap_endian(T u)
        {
            union {
                T u;
                unsigned char u8[sizeof(T)];
            } source, dest;
            source.u = u;
            for (size_t k = 0; k < sizeof(T); k++)
                dest.u8[k] = source.u8[sizeof(T) - k - 1];
            return dest.u;
        }

        void Skip(int n)
        {
            fseek(filePtr, n, SEEK_CUR);
        }

        template<typename T>
        void Skip()
        {
            Skip(sizeof(T));
        }

        template <typename T>
        void ReadArray(std::vector<T> &v, int byteLength)
        {
            fread(&v[0], sizeof(T), byteLength/sizeof(T), filePtr);
        }

        template<typename T>
        T ReadNumeric(bool swap = true)
        {
            T temp, value;
            temp = ReadStream<T>();

            if (swap)
                value = swap_endian<T>(temp);
            else
                value = temp;

            return value;
        }

        template<typename T>
        int ReadNumericPos()
        {
            int pos = ftell(filePtr);
            Skip<T>();
            return pos;
        }

        std::string ReadString(int length)
        {
            if (length < 1)
                return "";
            std::vector<char> fileBuffer(length);
            fread(&fileBuffer[0], 1, length, filePtr);
            std::string text(fileBuffer.begin(), fileBuffer.end());
            return text;
        }

        int ReadStringPos(int length)
        {
            int pos = ftell(filePtr);
            Skip(length);
            return pos;
        }

        int ReadPos()
        {
            return ftell(filePtr);
        }

        void GoTo(int pos)
        {
            fseek(filePtr, pos, SEEK_SET);
        }
    };
}

#endif //READDM_STREAMREADER_H
