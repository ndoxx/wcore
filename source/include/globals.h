#ifndef GLOBALS_H
#define GLOBALS_H

#include <cstdint>
#include <string>

#include "singleton.hpp"

namespace wcore
{

class GlobalData : public Singleton<GlobalData>
{
private:
    GlobalData (const GlobalData&){};
    GlobalData(){};
   ~GlobalData(){};

public:
    friend GlobalData& Singleton<GlobalData>::Instance();
    friend void Singleton<GlobalData>::Kill();

    // Screen
    uint32_t SCR_W    = 1024;
    uint32_t SCR_H    = 768;
    int WIN_W    = 1024;
    int WIN_H    = 768;
    bool     SCR_FULL = false;

    std::string START_LEVEL = "crystal";
};

#define GLB GlobalData::Instance()

}

#endif // GLOBALS_H
