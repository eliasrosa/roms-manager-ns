#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
# Targets PC (não precisam de devkitPro, não rodam no submake)
#---------------------------------------------------------------------------------
ifneq ($(MAKELEVEL),1)

# IP do Switch (modo nxlink) — alterar conforme sua rede
SWITCH_IP ?= 192.168.0.150
SWITCH_FTP_PORT ?= 5000

# IP desta máquina na rede — usado pelo modo debug para o Switch saber para
# onde mandar os logs em tempo real. Detectado automaticamente.
HOST_IP ?= $(shell ip route get 1 2>/dev/null | awk '{print $$7; exit}')

# Porta que o app usa para enviar logs (NXLINK_CLIENT_PORT do libnx)
NXLINK_LOG_PORT ?= 28771

# IP do servidor de sync (roms-manager-server), sobrescrito no envio.
#
# Vazio por padrão: o config.json versionado é enviado como está. Informe para
# apontar o app ao seu servidor sem precisar editar (e arriscar commitar) o
# config.json — ex.: make install-debug SERVER_IP=<ip-do-servidor>
SERVER_IP ?=

APP_DIR := /switch/roms-manager-ns

.PHONY: pc pc-setup pc-build clean-pc build watch serve deploy deploy-fresh \
        install install-fresh install-debug logs logs-live logs-clean \
        demo demo-switch

pc-setup:
	@if [ ! -d "build-pc" ]; then \
		cmake -B build-pc -DCMAKE_BUILD_TYPE=Release -DGLFW_BUILD_WAYLAND=OFF; \
	fi

pc: pc-setup
	@cmake --build build-pc -j$$(nproc)
	@echo ""
	@echo "Executavel: ./build-pc/roms-manager-ns"
	@./build-pc/roms-manager-ns

pc-build: pc-setup
	@cmake --build build-pc -j$$(nproc)
	@echo "Build PC concluido: ./build-pc/roms-manager-ns"

clean-pc:
	@echo Limpando build PC...
	@rm -fr build-pc

# Compila e abre o app demo do Borealis com galeria de todos os componentes
# disponiveis (buttons, cells, slider, images, recycler, etc).
# Util para descobrir o que a lib oferece antes de criar uma view nova.
demo:
	@if [ ! -d "library/build-demo" ]; then \
		echo "[1/2] Configurando demo..."; \
		cmake -B library/build-demo -S library \
			-DCMAKE_BUILD_TYPE=Release \
			-DPLATFORM_DESKTOP=ON \
			-DGLFW_BUILD_WAYLAND=OFF; \
	fi
	@echo "[2/2] Compilando demo..."
	@cmake --build library/build-demo -j$$(nproc)
	@echo ""
	@echo "Abrindo galeria de componentes do Borealis..."
	@cd library && ./build-demo/borealis_demo

# Mesma galeria de componentes, mas gerando o .nro e enviando pro Switch.
# Usa Dockerfile.demo (cross-compile do demo do Borealis) e instala num path
# proprio para nao se misturar com o app.
demo-switch:
	@echo "[1/3] Build do demo para Switch..."
	@docker build -f Dockerfile.demo -t borealis-demo .
	@echo "[2/3] Extraindo .nro..."
	@docker run --rm --user $$(id -u):$$(id -g) -v "$(CURDIR):/output" \
		--entrypoint "" borealis-demo cp /app/build/borealis_demo.nro /output/borealis-demo.nro
	@echo "[3/3] Enviando via nxlink (abra o hbmenu em modo nxlink, tecla Y)..."
	@pkill -f '[n]xlink' 2>/dev/null || true
	@docker run --rm --network host \
		-v $(CURDIR)/borealis-demo.nro:/app/borealis-demo.nro \
		devkitpro/devkita64 \
		nxlink -a $(SWITCH_IP) -p /switch/borealis-demo/borealis-demo.nro -s /app/borealis-demo.nro

watch:
	@./watch.sh

serve:
	@SERVER_PATH="../roms-manager-server"; \
	if [ ! -f "$$SERVER_PATH/docker-compose.yml" ]; then \
		echo "[erro] Servidor não encontrado em: $$SERVER_PATH"; \
		echo "       Clone o repositório primeiro:"; \
		echo "       git clone https://github.com/eliasrosa/roms-manager-server ../roms-manager-server"; \
		exit 1; \
	fi; \
	cd "$$SERVER_PATH" && docker compose up -d --build && \
	echo "Servidor rodando em http://localhost:8080"

deploy:
	@echo "=== Deploy via nxlink ==="
	@echo "Certifique-se que o Switch esta com hbmenu em modo nxlink (pressione Y)"
	@echo ""
	@pkill -f '[n]xlink' 2>/dev/null || true
	@if [ ! -f "roms-manager-ns.nro" ]; then \
		echo "[1/2] Gerando .nro..."; \
		./build.sh; \
	else \
		echo "[1/2] .nro ja existe (use 'make build' para regenerar)"; \
	fi
	@echo "[2/2] Enviando para Switch..."
	@if command -v nxlink > /dev/null 2>&1; then \
		nxlink -p /switch/roms-manager-ns/roms-manager-ns.nro -s ./roms-manager-ns.nro; \
	else \
		docker run --rm --network host \
			-v $(CURDIR)/roms-manager-ns.nro:/app/roms-manager-ns.nro \
			devkitpro/devkita64 \
			nxlink -a $(SWITCH_IP) -p /switch/roms-manager-ns/roms-manager-ns.nro -s /app/roms-manager-ns.nro; \
	fi

deploy-fresh: build deploy

install:
	@echo "=== Instalando no Switch via FTP ==="
	@echo "Certifique-se que o ftpd esta rodando no Switch"
	@echo ""
	@echo "[1/3] Criando diretorio..."
	@curl -s --ftp-create-dirs ftp://$(SWITCH_IP):$(SWITCH_FTP_PORT)/switch/roms-manager-ns/ > /dev/null || true
	@echo "[2/3] Enviando .nro..."
	@curl -T roms-manager-ns.nro ftp://$(SWITCH_IP):$(SWITCH_FTP_PORT)/switch/roms-manager-ns/roms-manager-ns.nro
	@echo "[3/3] Enviando config.json..."
	@if [ -n "$(SERVER_IP)" ]; then \
		sed 's/"host": "[^"]*"/"host": "$(SERVER_IP)"/' config.json > /tmp/rmns-config.json; \
		curl -T /tmp/rmns-config.json ftp://$(SWITCH_IP):$(SWITCH_FTP_PORT)/switch/roms-manager-ns/config.json; \
		rm -f /tmp/rmns-config.json; \
	else \
		curl -T config.json ftp://$(SWITCH_IP):$(SWITCH_FTP_PORT)/switch/roms-manager-ns/config.json; \
	fi
	@echo ""
	@echo "Instalado em sdmc:/switch/roms-manager-ns/"

install-fresh: build install

# Instala via FTP com o modo debug ligado: grava log em arquivo no SD e manda
# os logs em tempo real para esta máquina.
#
# Necessário porque o nxlink só entrega logs quando ele mesmo lança o app — ao
# instalar por FTP e abrir pelo hbmenu, o app precisa saber o IP do host.
install-debug:
	@if [ ! -f "roms-manager-ns.nro" ]; then \
		echo "[erro] roms-manager-ns.nro nao encontrado. Rode 'make build' primeiro."; \
		exit 1; \
	fi
	@if [ -z "$(HOST_IP)" ]; then \
		echo "[erro] Nao foi possivel detectar o IP local."; \
		echo "       Informe manualmente: make install-debug HOST_IP=192.168.0.4"; \
		exit 1; \
	fi
	@echo "=== Instalando no Switch em modo DEBUG ==="
	@echo "Switch:  $(SWITCH_IP)"
	@echo "Host:    $(HOST_IP) (recebe os logs na porta $(NXLINK_LOG_PORT))"
	@if [ -n "$(SERVER_IP)" ]; then echo "Servidor: $(SERVER_IP) (sobrescreve o config.json)"; fi
	@echo ""
	@echo "[1/3] Gerando config com debug habilitado..."
	@sed -e 's/"log_to_file": false/"log_to_file": true/' \
	     -e 's/"nxlink_host": ""/"nxlink_host": "$(HOST_IP)"/' \
	     config.json > /tmp/rmns-config-debug.json
	@if [ -n "$(SERVER_IP)" ]; then \
		sed -i 's/"host": "[^"]*"/"host": "$(SERVER_IP)"/' /tmp/rmns-config-debug.json; \
	fi
	@echo "[2/3] Enviando .nro..."
	@curl -s --ftp-create-dirs -T roms-manager-ns.nro \
		ftp://$(SWITCH_IP):$(SWITCH_FTP_PORT)$(APP_DIR)/roms-manager-ns.nro
	@echo "[3/3] Enviando config.json (debug on)..."
	@curl -s -T /tmp/rmns-config-debug.json \
		ftp://$(SWITCH_IP):$(SWITCH_FTP_PORT)$(APP_DIR)/config.json
	@rm -f /tmp/rmns-config-debug.json
	@echo ""
	@echo "Pronto. Agora:"
	@echo "  1. rode 'make logs-live' aqui"
	@echo "  2. abra o app no hbmenu do Switch"
	@echo "  3. depois use 'make logs' para baixar o arquivo completo"

# Escuta os logs em tempo real enviados pelo app (modo debug).
# -k mantem o listener ativo entre execucoes do app.
logs-live:
	@echo "=== Logs em tempo real ==="
	@echo "Escutando em $(HOST_IP):$(NXLINK_LOG_PORT) — Ctrl+C para sair"
	@echo "O app precisa ter debug.nxlink_host = $(HOST_IP) no config.json"
	@echo ""
	@nc -lk $(NXLINK_LOG_PORT)

# Baixa o arquivo de log do SD card. Funciona mesmo se o app morreu, e o .1 e a
# sessao anterior (util quando o app crasha e voce reabre para investigar).
logs:
	@mkdir -p .logs
	@echo "Baixando logs de $(SWITCH_IP)$(APP_DIR)/ ..."
	@if curl -s -f -o .logs/debug.log \
		ftp://$(SWITCH_IP):$(SWITCH_FTP_PORT)$(APP_DIR)/debug.log; then \
		echo "  -> .logs/debug.log"; \
	else \
		echo "[erro] debug.log nao encontrado no Switch."; \
		echo "       Verifique se debug.log_to_file esta true no config.json"; \
		echo "       (use 'make install-debug' para configurar automaticamente)"; \
		exit 1; \
	fi
	@if curl -s -f -o .logs/debug.log.1 \
		ftp://$(SWITCH_IP):$(SWITCH_FTP_PORT)$(APP_DIR)/debug.log.1 2>/dev/null; then \
		echo "  -> .logs/debug.log.1 (sessao anterior)"; \
	fi
	@echo ""
	@cat .logs/debug.log

logs-clean:
	@rm -rf .logs
	@echo "Logs locais removidos"

build:
	@./build.sh
endif

#---------------------------------------------------------------------------------
# A partir daqui: build Switch (requer DEVKITPRO)
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPRO)),)
# Se DEVKITPRO não estiver definido, só registrar targets Switch como erro
switch:
	$(error "Por favor configure DEVKITPRO no seu ambiente. export DEVKITPRO=<path to>/devkitpro")
all: switch
else

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# Configuração do app
#---------------------------------------------------------------------------------
APP_TITLE	:=	ROMs Manager NS
APP_AUTHOR	:=	elfarelo
APP_VERSION	:=	0.2.0

#---------------------------------------------------------------------------------
# TARGET é o nome do output
# BUILD é o diretório de objetos intermediários
# SOURCES é a lista de diretórios com código fonte
# DATA é a lista de diretórios com dados
# INCLUDES é a lista de diretórios com headers
# ROMFS é o diretório para RomFS (recursos do borealis)
#---------------------------------------------------------------------------------
TARGET		:=	roms-manager-ns
BUILD		:=	build.nx
SOURCES		:=	src src/views src/sync
DATA		:=	data
INCLUDES	:=	src

ROMFS		:=	resources
BOREALIS_PATH	:=	library

# Diretório de output de shaders (borealis/deko3d)
OUT_SHADERS	:=	shaders

#---------------------------------------------------------------------------------
# Opções de compilação
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS	:=	-g -Wall -O2 -ffunction-sections \
			$(ARCH) $(DEFINES)

CFLAGS	+=	$(INCLUDE) -D__SWITCH__

CXXFLAGS	:= $(CFLAGS) -std=gnu++1z -Wno-volatile -include cstdint -include optional

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -lnx

#---------------------------------------------------------------------------------
# Diretórios de bibliotecas
#---------------------------------------------------------------------------------
LIBDIRS	:= $(PORTLIBS) $(LIBNX)

#---------------------------------------------------------------------------------
# Incluir borealis.mk (adiciona sources, includes, libs do borealis)
#---------------------------------------------------------------------------------
include $(TOPDIR)/$(BOREALIS_PATH)/library/borealis.mk

#---------------------------------------------------------------------------------
# Regras de build (não editar abaixo)
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

#---------------------------------------------------------------------------------
# Shader compilation (GLSL -> DKSH para deko3d/nanovg)
#---------------------------------------------------------------------------------
GLSLFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.glsl)))

ifneq ($(strip $(ROMFS)),)
	ROMFS_TARGETS :=
	ROMFS_FOLDERS :=
	ifneq ($(strip $(OUT_SHADERS)),)
		ROMFS_SHADERS := $(ROMFS)/$(OUT_SHADERS)
		ROMFS_TARGETS += $(patsubst %.glsl, $(ROMFS_SHADERS)/%.dksh, $(GLSLFILES))
		ROMFS_FOLDERS += $(ROMFS_SHADERS)
	endif
	export ROMFS_DEPS := $(foreach file,$(ROMFS_TARGETS),$(CURDIR)/$(file))
endif

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.jpg)
	ifneq (,$(findstring $(TARGET).jpg,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).jpg
	else
		ifneq (,$(findstring icon.jpg,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.jpg
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_ICON)),)
	export NROFLAGS += --icon=$(APP_ICON)
endif

ifeq ($(strip $(NO_NACP)),)
	export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
endif

ifneq ($(APP_TITLEID),)
	export NACPFLAGS += --titleid=$(APP_TITLEID)
endif

ifneq ($(ROMFS),)
	export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)
endif

.PHONY: $(BUILD) clean all switch clean-all

switch: $(BUILD)
all: $(BUILD)

$(BUILD): $(ROMFS_TARGETS)
	@[ -d $@ ] || mkdir -p $@
	@MSYS2_ARG_CONV_EXCL="-D;$(MSYS2_ARG_CONV_EXCL)" $(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
# Regras de compilação de shaders
#---------------------------------------------------------------------------------
ifneq ($(strip $(ROMFS_TARGETS)),)

$(ROMFS_TARGETS): | $(ROMFS_FOLDERS)

$(ROMFS_FOLDERS):
	@mkdir -p $@

$(ROMFS_SHADERS)/%_vsh.dksh: %_vsh.glsl
	@echo {vert} $(notdir $<)
	@uam -s vert -o $@ $<

$(ROMFS_SHADERS)/%_tcsh.dksh: %_tcsh.glsl
	@echo {tess_ctrl} $(notdir $<)
	@uam -s tess_ctrl -o $@ $<

$(ROMFS_SHADERS)/%_tesh.dksh: %_tesh.glsl
	@echo {tess_eval} $(notdir $<)
	@uam -s tess_eval -o $@ $<

$(ROMFS_SHADERS)/%_gsh.dksh: %_gsh.glsl
	@echo {geom} $(notdir $<)
	@uam -s geom -o $@ $<

$(ROMFS_SHADERS)/%_fsh.dksh: %_fsh.glsl
	@echo {frag} $(notdir $<)
	@uam -s frag -o $@ $<

$(ROMFS_SHADERS)/%.dksh: %.glsl
	@echo {comp} $(notdir $<)
	@uam -s comp -o $@ $<

endif

clean:
	@echo Limpando...
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf

clean-all: clean clean-pc

else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

all	:	$(OUTPUT).nro

$(OUTPUT).nro	:	$(OUTPUT).elf $(OUTPUT).nacp

$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

-include $(DEPENDS)

endif

# Fecha o ifeq DEVKITPRO
endif
