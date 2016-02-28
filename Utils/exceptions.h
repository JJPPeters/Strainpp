#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>

class SizeException : public std::exception
{
    virtual const char* what() const throw()
      {
        return "Image dimensions do not match array dimensions";
      }
};

extern SizeException sizeError;

#endif // EXCEPTIONS_H
