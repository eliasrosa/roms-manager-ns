-- Toolchain devkitA64 para Nintendo Switch
toolchain("devkita64")
    set_kind("standalone")
    set_sdkdir("$(env DEVKITPRO)/devkitA64")

    set_toolset("cc", "aarch64-none-elf-gcc")
    set_toolset("cxx", "aarch64-none-elf-g++")
    set_toolset("ld", "aarch64-none-elf-g++")
    set_toolset("ar", "aarch64-none-elf-ar")
    set_toolset("strip", "aarch64-none-elf-strip")
    set_toolset("as", "aarch64-none-elf-as")

    on_load(function (toolchain)
        local devkitpro = os.getenv("DEVKITPRO") or "/opt/devkitpro"
        local devkita64 = devkitpro .. "/devkitA64"
        local libnx = devkitpro .. "/libnx"
        local portlibs = devkitpro .. "/portlibs/switch"

        toolchain:add("cxflags", "-march=armv8-a+crc+crypto", "-mtune=cortex-a57", "-mtp=soft", "-fPIE")
        toolchain:add("cxflags", "-D__SWITCH__", "-DSWITCH")
        toolchain:add("cxflags", "-I" .. libnx .. "/include")
        toolchain:add("cxflags", "-I" .. portlibs .. "/include")
        toolchain:add("ldflags", "-specs=" .. libnx .. "/switch.specs")
        toolchain:add("ldflags", "-L" .. libnx .. "/lib", "-L" .. portlibs .. "/lib")
        toolchain:add("ldflags", "-lnx")
    end)
toolchain_end()
