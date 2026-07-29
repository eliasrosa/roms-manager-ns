#pragma once

/**
 * Platform - Abstração de caminhos entre Switch e PC
 *
 * No Switch: usa sdmc:/ (SD card)
 * No PC: usa ./test_sd/ (pasta local para simulação)
 */

#include <string>

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

} // namespace platform
