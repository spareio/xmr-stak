#include "RegistryHelper.h"

#include <Windows.h>

bool RegistryHelper::matchBKey(const std::string& bKey)
{
    // Reg key checker
    DWORD dwType = REG_SZ;
    HKEY hKey = 0;
    char value[1024];
    DWORD value_length = 1024;
    const char* subkey = "SOFTWARE\\WOW6432Node\\Spareio\\SpareioApp";
    RegOpenKey(HKEY_LOCAL_MACHINE, subkey, &hKey);
    RegQueryValueEx(hKey, "bKey", NULL, &dwType, (LPBYTE)&value, &value_length);

    if (ERROR_SUCCESS != 0)
    {
        return false;
    }
    else if (std::string key(value); key != bKey)
    {
        return false;
    }

    return true;
}