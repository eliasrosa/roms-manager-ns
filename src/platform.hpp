#pragma once

/**
 * Platform - Abstração de caminhos entre Switch e PC
 *
 * No Switch: usa sdmc:/ (SD card)
 * No PC: usa ./test_sd/ (pasta local para simulação)
 */

#include <string>

#ifdef __SWITCH__
#include <switch.h>
#else
#include <sys/statvfs.h>
#endif

namespace platform {

#ifdef __SWITCH__

inline std::string sdRoot()
{
    return "sdmc:/";
}

inline std::string romsPath()
{
    return "sdmc:/roms/";
}

#else

inline std::string sdRoot()
{
    return "./test_sd/";
}

inline std::string romsPath()
{
    return "./test_sd/roms/";
}

#endif

/**
 * Informações de espaço de um storage.
 * Retorna valores em GB. Retorna 0 em caso de erro.
 */
struct StorageInfo
{
    double totalGB;
    double freeGB;
    double usedGB;
    int usagePercent; // 0-100
};

inline StorageInfo getSdInfo()
{
    StorageInfo info = {0.0, 0.0, 0.0, 0};

#ifdef __SWITCH__
    nsInitialize();
    s64 total = 0, free = 0;
    if (R_SUCCEEDED(nsGetTotalSpaceSize(NcmStorageId_SdCard, &total)) &&
        R_SUCCEEDED(nsGetFreeSpaceSize(NcmStorageId_SdCard, &free)))
    {
        info.totalGB = static_cast<double>(total) / (1024.0 * 1024.0 * 1024.0);
        info.freeGB = static_cast<double>(free) / (1024.0 * 1024.0 * 1024.0);
        info.usedGB = info.totalGB - info.freeGB;
    }
    nsExit();
#else
    struct statvfs stat;
    if (statvfs(sdRoot().c_str(), &stat) == 0)
    {
        double totalBytes = static_cast<double>(stat.f_blocks) * stat.f_frsize;
        double freeBytes = static_cast<double>(stat.f_bfree) * stat.f_frsize;
        info.totalGB = totalBytes / (1024.0 * 1024.0 * 1024.0);
        info.freeGB = freeBytes / (1024.0 * 1024.0 * 1024.0);
        info.usedGB = info.totalGB - info.freeGB;
    }
#endif

    if (info.totalGB > 0.0)
        info.usagePercent = static_cast<int>((info.usedGB / info.totalGB) * 100.0);

    return info;
}

inline StorageInfo getSystemInfo()
{
    StorageInfo info = {0.0, 0.0, 0.0, 0};

#ifdef __SWITCH__
    nsInitialize();
    s64 total = 0, free = 0;
    if (R_SUCCEEDED(nsGetTotalSpaceSize(NcmStorageId_BuiltInUser, &total)) &&
        R_SUCCEEDED(nsGetFreeSpaceSize(NcmStorageId_BuiltInUser, &free)))
    {
        info.totalGB = static_cast<double>(total) / (1024.0 * 1024.0 * 1024.0);
        info.freeGB = static_cast<double>(free) / (1024.0 * 1024.0 * 1024.0);
        info.usedGB = info.totalGB - info.freeGB;
    }
    nsExit();
#else
    // No PC, simula com o filesystem raiz
    struct statvfs stat;
    if (statvfs("/", &stat) == 0)
    {
        double totalBytes = static_cast<double>(stat.f_blocks) * stat.f_frsize;
        double freeBytes = static_cast<double>(stat.f_bfree) * stat.f_frsize;
        info.totalGB = totalBytes / (1024.0 * 1024.0 * 1024.0);
        info.freeGB = freeBytes / (1024.0 * 1024.0 * 1024.0);
        info.usedGB = info.totalGB - info.freeGB;
    }
#endif

    if (info.totalGB > 0.0)
        info.usagePercent = static_cast<int>((info.usedGB / info.totalGB) * 100.0);

    return info;
}

} // namespace platform
