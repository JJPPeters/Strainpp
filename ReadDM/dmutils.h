//
// Created by Jon on 10/05/2015.
//

#ifndef READDM_UTILS_H
#define READDM_UTILS_H

#include <sstream>
#include <cstring>
#include <stdint.h>

namespace Utils
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }

    bool TestEndian(){
        uint32_t i = 0x12345678;
        char ch[4];
        memcpy( ch, &i, 4 );
        //returns 1 if the machine is little endian
        bool bLittleEndian = ch[0] == 0x78;
        return bLittleEndian;
    }

    std::string RemoveTagName(const std::string& tagName)
    {
        size_t lastdot = tagName.find_last_of(".");
        if (lastdot == std::string::npos) return tagName;
        return tagName.substr(0, lastdot);
    }

}

#endif //READDM_UTILS_H
