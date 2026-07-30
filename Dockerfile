# Dockerfile para build do ROMs Manager NS (Switch)
# Usa devkitPro + CMake para cross-compile
FROM devkitpro/devkita64:latest

# Instalar cmake
RUN apt-get update && apt-get install -y cmake build-essential && apt-get clean

# PATH com tools do devkitPro
ENV PATH="/opt/devkitpro/devkitA64/bin:/opt/devkitpro/tools/bin:${PATH}"

# Instalar pacotes Switch
RUN dkp-pacman -Syyu --noconfirm && \
    dkp-pacman -S --noconfirm \
        switch-glfw \
        switch-mesa \
        switch-glm \
        deko3d \
    && dkp-pacman -Scc --noconfirm

WORKDIR /app

# Copiar código fonte
COPY . .

# Garantir que o submodule borealis está presente
RUN if [ ! -f "library/library/CMakeLists.txt" ]; then \
        echo "ERRO: submodule borealis não encontrado em library/" && \
        echo "Execute antes: git submodule update --init --recursive" && \
        exit 1; \
    fi

# Compilar shaders (GLSL -> DKSH para deko3d/nanovg)
RUN mkdir -p resources/shaders && \
    uam -s vert -o resources/shaders/fill_vsh.dksh library/library/lib/extern/nanovg/deko3d/shaders/fill_vsh.glsl && \
    uam -s frag -o resources/shaders/fill_fsh.dksh library/library/lib/extern/nanovg/deko3d/shaders/fill_fsh.glsl && \
    uam -s frag -o resources/shaders/fill_aa_fsh.dksh library/library/lib/extern/nanovg/deko3d/shaders/fill_aa_fsh.glsl && \
    echo "Shaders compilados OK"

# Build Switch via CMake cross-compile
RUN cmake -B build.nx \
        -DCMAKE_TOOLCHAIN_FILE=cmake/SwitchToolchain.cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DPLATFORM_SWITCH=ON \
        -DPLATFORM_DESKTOP=OFF \
        -DBOREALIS_USE_DEKO3D=ON \
        -DBRLS_UNITY_BUILD=OFF && \
    cmake --build build.nx -j$(nproc)

# Gerar .nro (elf2nro)
RUN aarch64-none-elf-strip -s build.nx/roms-manager-ns -o build.nx/roms-manager-ns.stripped && \
    nacptool --create "ROMs Manager NS" "farelo" "0.2.0" build.nx/roms-manager-ns.nacp && \
    elf2nro build.nx/roms-manager-ns.stripped build.nx/roms-manager-ns.nro \
        --nacp=build.nx/roms-manager-ns.nacp \
        --romfsdir=resources
