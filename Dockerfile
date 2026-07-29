# Dockerfile para build do ROMs Manager NS (versão Borealis)
# Usa imagem oficial do devkitPro com toolchain Nintendo Switch
FROM devkitpro/devkita64:latest

# Instalar dependências do Borealis (deko3d, mesa, glfw para shaders)
RUN dkp-pacman -Syyu --noconfirm && \
    dkp-pacman -S --noconfirm \
        switch-glfw \
        switch-mesa \
        switch-glm \
        deko3d \
    && dkp-pacman -Scc --noconfirm

WORKDIR /app

# Copiar código fonte (submodules já devem estar presentes)
COPY . .

# Garantir que o submodule borealis está presente
RUN if [ ! -f "library/library/borealis.mk" ]; then \
        echo "ERRO: submodule borealis não encontrado em library/" && \
        echo "Execute antes: git submodule update --init --recursive" && \
        exit 1; \
    fi

# Build (target Switch)
# Workaround: swkbd API mudou no libnx mais novo, excluir arquivo
RUN sed -i 's|swkbdConfigSetStringLenMaxExt|swkbdConfigSetStringLenMax|g' \
    library/library/lib/platforms/switch/swkbd.cpp && \
    make switch

# O output (.nro) fica em /app/roms-manager-ns.nro
