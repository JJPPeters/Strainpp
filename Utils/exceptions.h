#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>

class SizeException : public std::exception
{
    const char* what() const throw() override
    {
        return "Image dimensions do not match array dimensions";
    }
};

extern SizeException sizeError;

#endif // EXCEPTIONS_H
