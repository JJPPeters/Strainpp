//
// Created by Jon on 10/05/2015.
//

#ifndef READDM_DMREADER_H
#define READDM_DMREADER_H
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <ctime>


#include "tagreader.h"

namespace DMRead
{

    class DMReader
    {
    public:

        DMReader(std::string filePath)
        {
            Reader.setStream(filePath);

            version = Reader.ReadNumeric<uint32_t>();

            if (version == 3)
            {
                TagReader<uint32_t> DMFile(TagData);
            }
            else if (version == 4)
            {
                TagReader<uint64_t> DMFile(TagData);
            }
            else
                throw std::runtime_error("Unsupported version of Digital Micrograph file.");
        }

        ~DMReader()
        {
            Reader.closeStream();
        }

        void printTags()
        {
            for (auto const &it : TagData)
            {
                std::cout << it.first << " " << std::endl;
                if (it.first == "root.ImageList.1.Name")
                    std::cout << std::get<0>(it.second) << std::endl;
            }
        }

        float getScale()
        {
            return ReadValue<float>("root.ImageList.1.ImageData.Calibrations.Dimension.0.Scale");
        }

        std::string getUnits()
        {
            std::vector<char> charString = ReadArray<char>("root.ImageList.1.ImageData.Calibrations.Dimension.0.Units");
            return std::string( charString.begin(), charString.end() );
        }

        std::vector<double> getImage(int offset = 0, int length = -1)
        {
            return ReadArray<double>("root.ImageList.1.ImageData.Data", offset, length);
        }

        int getX()
        {
            return ReadValue<uint32_t>("root.ImageList.1.ImageData.Dimensions.0");
        }

        int getY()
        {
            return ReadValue<uint32_t>("root.ImageList.1.ImageData.Dimensions.1");
        }

        int getZ()
        {
            try
            {
                return ReadValue<uint32_t>("root.ImageList.1.ImageData.Dimensions.2");
            }
            catch(std::exception &e)
            {
                return 1;
            }
        }

        void close()
        {
            Reader.closeStream();
        }

        bool isRGB() {
            bool is_rgb = true;
            try {
                // RGB get set as int32, this tag is an easy identifier (also red and blue equivalent tags)
                TSL rgb_tag = GetTag("root.Green Gain");
            } catch(...) {
                is_rgb = false;
            }

            return is_rgb;
        }

        bool isComplex() {
            bool is_complex = true;
            try {
                // in complex, Data is split into 0 and 1 sub images (I assume real and imaginary)
                TSL complex_tag = GetTag("root.ImageList.1.ImageData.Data.0");
            } catch(...) {
                is_complex = false;
            }

            return is_complex;
        }

    private:
        const bool swapEndian = false;

        TypeList enumInst;

        std::map<std::string, TSL> TagData;

        int version;

        TSL GetTag(std::string SearchName)
        {
            for (auto const &it : TagData)
            {
                if (it.first == SearchName)
                {
                    return it.second;
                }
            }

            throw std::runtime_error("Can't find tag.");
        }

        template <typename T>
        T ReadValue(std::string TagName)
        {
            TSL valTag;
            try
            {
                valTag = GetTag(TagName);
            }
            catch(std::exception &e)
            {
                throw e;
            }

            int64_t data_type = std::get<0>(valTag);
            int64_t data_position = std::get<2>(valTag);

            T Output;
            Reader.GoTo(data_position);
            switch(data_type)
            {
                case int8:
                    Output = (T)Reader.ReadNumeric<int8_t>(swapEndian);
                    break;
                case int16:
                    Output = (T)Reader.ReadNumeric<int16_t>(swapEndian);
                    break;
                case int32:
                    Output = (T)Reader.ReadNumeric<int32_t>(swapEndian);
                    break;
                case uint8:
                    Output = (T)Reader.ReadNumeric<uint8_t>(swapEndian);
                    break;
                case uint16:
                    Output = (T)Reader.ReadNumeric<uint16_t>(swapEndian);
                    break;
                case uint32:
                    Output = (T)Reader.ReadNumeric<uint32_t>(swapEndian);
                    break;
                case float32:
                    Output = (T)Reader.ReadNumeric<float>(swapEndian);
                    break;
                case float64:
                    Output = (T)Reader.ReadNumeric<double>(swapEndian);
                    break;
                case int64:
                    Output = (T)Reader.ReadNumeric<int64_t>(swapEndian);
                    break;
                case uint64:
                    Output = (T)Reader.ReadNumeric<uint64_t>(swapEndian);
                    break;
                default:
                    throw std::runtime_error("Cannot read datatype");

            }
            return Output;
        }

        template <typename T>
        std::vector<T> ReadArray(std::string TagName, int offset = 0, int length = -1)
        {

            // do this because RGB gives the same datatype as int32...
            if (isRGB()) {
                throw std::runtime_error("RGB images are not supported.");
            }

            if (isComplex()) {
                throw std::runtime_error("Complex images are not supported.");
            }

            TSL arrayTag = GetTag(TagName);
            int64_t data_type = std::get<0>(arrayTag);
            int64_t data_size = std::get<1>(arrayTag);
            int64_t data_position = std::get<2>(arrayTag);

            if (length < 1)
                length = data_size;

            if (offset < 0)
                throw std::runtime_error("Invalid array offset.");

            if (offset + length > data_size)
                throw std::runtime_error("Array selection out of bounds.");

            std::vector<T> output(data_size);

            switch(data_type)
            {
                case int8:
                    _ReadArray<T, int8_t>(output, data_type, data_position+offset, length);
                    break;
                case int16:
                    _ReadArray<T, int16_t>(output, data_type, data_position+offset, length);
                    break;
                case int32:
                    _ReadArray<T, int32_t>(output, data_type, data_position+offset, length);
                    break;
                case uint8:
                    _ReadArray<T, uint8_t>(output, data_type, data_position+offset, length);
                    break;
                case uint16:
                    _ReadArray<T, uint16_t>(output, data_type, data_position+offset, length);
                    break;
                case uint32:
                    _ReadArray<T, uint32_t>(output, data_type, data_position+offset, length);
                    break;
                case float32:
                    _ReadArray<T, float>(output, data_type, data_position+offset, length);
                    break;
                case float64:
                    _ReadArray<T, double>(output, data_type, data_position+offset, length);
                    break;
                case int64:
                    _ReadArray<T, int64_t>(output, data_type, data_position+offset, length);
                    break;
                case uint64:
                    _ReadArray<T, uint64_t>(output, data_type, data_position+offset, length);
                    break;
                default:
                    throw std::runtime_error("Cannot open datatype");
            }
            return output;
        }

        template <typename T, typename X>
        void _ReadArray(std::vector<T> &data, int type, int position, int size)
        {
            Reader.GoTo(position);
//            for (int i = 0; i < size; ++i)
//            {
//                data[i] = (T)Reader.ReadNumeric<X>(swapEndian);
//            }
            std::vector<X> buffer(size);
            Reader.ReadArray(buffer, size*sizeof(X));
            #pragma omp parallel for
            for (int i = 0; i < size; ++i)
                data[i] = (T)buffer[i];
        }

    };
}

#endif //READDM_DMREADER_H
