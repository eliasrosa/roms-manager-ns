-- ROMs Manager NS - xmake.lua
-- Build para Switch (cross-compile com devkitA64)

add_repositories("switch-repo https://github.com/PoloNX/switch-repo.git")

includes("toolchain/*.lua")

add_defines(
    'BRLS_RESOURCES="romfs:/"',
    "YG_ENABLE_EVENTS",
    "STBI_NO_THREAD_LOCALS",
    "BOREALIS_USE_DEKO3D"
)

add_rules("mode.debug", "mode.release")

add_requires("borealis", {repo = "switch-repo"}, "deko3d")

target("roms-manager-ns")
    set_kind("binary")
    if not is_plat("cross") then
        return
    end

    set_arch("aarch64")
    add_rules("switch")
    set_toolchains("devkita64")
    set_languages("c++17")

    set_values("switch.name", "ROMs Manager NS")
    set_values("switch.author", "farelo")
    set_values("switch.version", "0.2.0")
    set_values("switch.romfs", "resources")

    add_files("src/*.cpp")
    add_files("src/views/*.cpp")
    add_files("src/sync/*.cpp")
    add_includedirs("src")
    add_packages("borealis", "deko3d")
