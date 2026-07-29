# ROMS-005 — Arquitetura do Projeto

## Visão Geral

ROMs Manager NS é um app homebrew para Nintendo Switch que gerencia ROMs (jogos) no SD card, com interface gráfica nativa e sincronização WiFi com servidor local.

## Objetivos

1. Interface gráfica estilo Switch nativo (não console text-mode)
2. Compilar tanto para Switch quanto para PC (testes sem console)
3. Sincronizar ROMs/covers/saves via WiFi com servidor no PC
4. Sem dependências externas pesadas — binário leve e portável
5. Workflow de desenvolvimento rápido (hot reload no PC, deploy via nxlink)

## Público-Alvo

- Usuários de Switch com CFW (Atmosphère) que querem gerenciar ROMs de forma organizada
- Workflow: PC como servidor de arquivos → Switch baixa via WiFi

## Restrições

- **Hardware**: Nintendo Switch (Tegra X1, ARM64, 4GB RAM)
- **SO**: HorizonOS via Atmosphère (libnx como SDK)
- **GPU**: deko3d (API nativa) ou OpenGL via mesa
- **Rede**: WiFi (802.11ac), sem garantia de banda
- **Storage**: SD card (FAT32/exFAT), arquivos de até 16GB
- **Toolchain**: devkitPro / devkitA64 / GCC cross-compile
