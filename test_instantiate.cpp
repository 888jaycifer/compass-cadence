#include <windows.h>
#include <iostream>

int main()
{
    std::cout << "Loading Compass Cadence.vst3..." << std::endl;
    HMODULE hMod = LoadLibraryA("C:\\Users\\jjntw\\Desktop\\MUSIC_boot\\VST_customs\\compass-cadence\\build\\CompassCadence_artefacts\\Debug\\VST3\\Compass Cadence.vst3\\Contents\\x86_64-win\\Compass Cadence.vst3");
    if (!hMod)
    {
        std::cerr << "Failed to load DLL, error: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "DLL loaded successfully!" << std::endl;

    auto getPluginFactory = GetProcAddress(hMod, "GetPluginFactory");
    std::cout << "GetPluginFactory: " << (void*)getPluginFactory << std::endl;

    FreeLibrary(hMod);
    return 0;
}
