#pragma once
#include <string>

//protecao por mutex interna
//simultaneo

namespace Logger{
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
}