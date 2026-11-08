# Guia para Agentes de IA: Port Nativo de Pokémon HeartGold / SoulSilver

Este repositório contém a descompilação completa em C/C++ de **Pokémon HeartGold e SoulSilver**.
O objetivo desta branch (`native-port`) é implementar a **recompilação nativa para PC (Linux, Windows, macOS)**, gerando um executável que execute o jogo diretamente no sistema operacional do usuário com suporte a periféricos modernos (teclado, controle, mouse para toque), sem necessidade de emuladores.

---

## 1. Visão Geral da Arquitetura

O jogo original foi desenvolvido para o hardware do **Nintendo DS**:
- **Dois processadores**: ARM946E-S (lógica de jogo e gráficos) e ARM7TDMI (som, touchscreen, rádio/Wi-Fi, RTC).
- **Duas telas**: Tela Superior (Principal) e Tela Inferior (Sub/Touchscreen), com resolução de 256x192 cada.
- **NitroSDK & NitroSystem**: Conjunto de bibliotecas de baixo nível da Nintendo para controle de interrupções (`OS`), cartucho/arquivos (`FS`/`CARD`), vídeo 2D/3D (`GX`/`G2`/`G3`) e áudio (`SND`).
- **Sistema de Overlays**: Módulos de código carregados dinamicamente em RAM conforme a cena (ex: batalha, campo, pokegear, minigames).

### Estrutura do Repositório

- `src/`: Lógica do jogo (movimentação, batalha, menus, eventos, scripts, subsistemas).
- `include/`: Cabeçalhos do jogo e declarações de tipos globais.
- `lib/NitroSDK/`: Implementação decompilada da SDK do Nintendo DS.
- `lib/NitroSystem/` e `lib/NitroDWC/`: Middleware de animação, modelos 3D, som e rede.
- `files/`: Sistema de arquivos do jogo (gráficos, mapas, textos, NARCs).
- `port/` *(a ser criado)*: Camada de abstração nativa (HAL), janela, áudio, renderização e ponto de entrada da versão PC.

---

## 2. Roteiro de Implementação (Roadmap)

### Fase 1: Pipeline de Build Nativo e Tipos Base
- [ ] **Configuração do CMake**: Criar um `CMakeLists.txt` moderno para compilar os fontes em C/C++ usando compiladores modernos (GCC, Clang, MSVC) para x86_64 e ARM64.
- [ ] **Normalização de Tipos e Compatibilidade 64-bit**:
  - Assegurar que tipos como `u32`, `s32`, `u16`, `s16`, `u8`, `s8`, `BOOL` mantenham seus tamanhos exatos.
  - Identificar e tratar casts perigosos entre ponteiros (64-bit no PC) e inteiros (32-bit no DS).
- [ ] **Definição de macros de plataforma**: Introduzir `#ifdef PLATFORM_NATIVE` para isolar código dependente de hardware do DS sem poluir o código original.

### Fase 2: Sistema Operacional e Sistema de Arquivos (HAL Base)
- [ ] **Abstração de OS (`OS_`)**:
  - Implementar gerenciamento de threads, mutexes e filas de mensagens nativas (usando `std::thread`/`std::mutex` ou threads do SDL).
  - Emular o relógio do sistema (`OS_GetTick`, `OS_GetTime`) e interrupções virtuais (VBlank / HBlank a 60 FPS).
- [ ] **Sistema de Arquivos Virtual (`FS_` e `CARD_`)**:
  - Redirecionar as chamadas de leitura de arquivos e arquivos NARC para o diretório local `files/` ou para um arquivo compactado de assets.
  - Implementar suporte a save game local (armazenar o arquivo de save em disco em substituição à Flash/SRAM do cartucho).

### Fase 3: Janela, Entrada e Áudio (SDL2 / SDL3)
- [ ] **Gerenciamento de Janelas e Telas**:
  - Criar janela via SDL renderizando as duas telas (256x192 cada): modos vertical (uma acima da outra), horizontal (lado a lado) ou híbrido.
- [ ] **Mapeamento de Entrada (`PAD_`)**:
  - Capturar teclado e gamepads modernos, traduzindo para os botões do DS (A, B, X, Y, D-Pad, L, R, Start, Select).
  - Emular a Touchscreen mapeando cliques e toques do mouse para as coordenadas [0..255, 0..191] da tela inferior.
- [ ] **Backend de Áudio (`SND_`)**:
  - Encapsular os canais de áudio da NitroSDK em um buffer de saída SDL Audio / miniaudio.

### Fase 4: Pipeline Gráfico (2D e 3D)
- [ ] **Motor Gráfico 2D (`GX_`, `G2_`)**:
  - Emulação dos planos de background (BG0, BG1, BG2, BG3) em modos texto e afim.
  - Renderização de Sprites/OAM e paletas de cores.
- [ ] **Motor Gráfico 3D (`G3_`, `G3X_`)**:
  - Suporte ao pipeline de geometria de vértice fixo e rasterização (renderizador por software ou tradução para OpenGL ES / OpenGL moderno).

### Fase 5: Integração de Overlays e ARM7
- [ ] **Carregamento de Overlays**:
  - Adaptar os overlays de código para compilação estática unificada (monolítica) ou bibliotecas dinâmicas carregadas sob demanda.
- [ ] **Coprocessador ARM7**:
  - Integrar os módulos de serviço do ARM7 (som, RTC, interface) para executarem em thread de apoio ou reimplementação em alto nível (HLE).

---

## 3. Diretrizes para Agentes de IA

Ao atuar nesta base de código, todo agente deve seguir estritamente as seguintes regras:

1. **Modularidade e Não-Destrutividade:**
   - Todo o código novo de compatibilidade nativa deve residir dentro do diretório `port/` (ex: `port/hal/`, `port/graphics/`, `port/audio/`, `port/input/`).
   - Evitar modificar o código-fonte original de `src/` a menos que seja estritamente necessário (como ajustar casts para 64-bit ou incluir macros `#ifdef PLATFORM_NATIVE`).
2. **Segurança de Memória e 64-bit:**
   - Não assumir `sizeof(void*) == sizeof(uint32_t)`. Use `uintptr_t` quando conversões de ponteiros forem indispensáveis.
   - Respeitar alinhamentos e endianness (o Nintendo DS é Little Endian).
3. **Preservação de Documentação:**
   - Manter todos os comentários existentes e documentar claramente cada nova interface da camada nativa.
4. **Commits Pequenos e Testáveis:**
   - Desenvolver incrementalmente, validando cada subsistema (ex: Sistema de Arquivos -> Janela -> Gráficos 2D -> Input -> Áudio).
