//
// Created by brota on 15.09.2025.
//

#ifndef ERRORFORMATER_H
#define ERRORFORMATER_H
#include <string>

#include "components/AboutDialog.h"

class ErrorFormater
{
public:
    static std::string GetLastErrorString();
    static std::string GetErrorString(DWORD err);
};

#endif //ERRORFORMATER_H
