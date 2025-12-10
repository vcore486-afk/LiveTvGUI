#include "pythonmanager.h"
#include <QDir>
#include <QDebug>
#include <QStandardPaths>

PythonManager& PythonManager::instance() {
    static PythonManager inst;
    return inst;
}

PythonManager::PythonManager() {
    if (!initialized) {
        Py_Initialize();

        // Получение пути к домашнему каталогу и создание пути к папке .livetv
        QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        QString livetvPath = homePath + "/.livetv";

        // Добавляем путь к папке .livetv
        PyRun_SimpleString("import sys, os");
        PyRun_SimpleString(("sys.path.append('" + livetvPath + "')").toUtf8().data());

        initialized = true;
        qDebug() << "[PythonManager] Python initialized";
    }
}

PythonManager::~PythonManager() {
    if (initialized) {
        Py_Finalize();
        qDebug() << "[PythonManager] Python finalized";
    }
}

bool PythonManager::importModule(const char* name) {
    PyObject* pName = PyUnicode_FromString(name);
    PyObject* pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (!pModule) {
        PyErr_Print();
        return false;
    }

    Py_DECREF(pModule);
    return true;
}

QString PythonManager::callFunction(const char* module, const char* function, const char* argument)
{
    PyObject* pName = PyUnicode_FromString(module);
    PyObject* pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (!pModule) {
        PyErr_Print();
        qDebug() << "Модуль не импортирован!";
        return "Ошибка импорта модуля";
    }

    PyObject* pFunc = PyObject_GetAttrString(pModule, function);

    if (!pFunc || !PyCallable_Check(pFunc)) {
        Py_DECREF(pModule);
        PyErr_Print();
        qDebug() << "Функция не найдена или не вызываемая!";
        return "Функция не найдена или не вызываемая";
    }

    // Создаем объект с аргументом для Python
    PyObject* pyArgs = Py_BuildValue("(s)", argument);

    // Вызываем функцию с аргументами
    PyObject* pResult = PyObject_CallObject(pFunc, pyArgs);
    Py_DECREF(pyArgs);
    Py_DECREF(pFunc);
    Py_DECREF(pModule);

    if (!pResult) {
        PyErr_Print();
        qDebug() << "Ошибка выполнения функции!";
        return "Ошибка выполнения функции";
    }

    // Проверяем тип возвращенного значения
    if (PyUnicode_Check(pResult)) {
        QString result = QString::fromUtf8(PyUnicode_AsUTF8(pResult));
        Py_DECREF(pResult);
        return result;
    } else {
        Py_DECREF(pResult);
        return "Возвращенное значение не является строкой!";
    }
}