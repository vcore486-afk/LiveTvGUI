
#pragma once
#undef slots  
#include <Python.h>  
#include <QString>


class PythonManager {
public:
    static PythonManager& instance();

    bool importModule(const char* name);
    QString callFunction(const char* module, const char* function);

private:
    PythonManager();
    ~PythonManager();

    bool initialized = false;
};
