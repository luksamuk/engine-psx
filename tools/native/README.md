# Native Build Tools para engine-psx

Ferramentas nativas em C para build de assets do engine-psx, substituindo as versões Python originais para maior velocidade.

## Status do Projeto

| Ferramenta | Status | CLI | Descrição |
|------------|--------|-----|-----------|
| **cooklvl** | ✅ Pronto | `cooklvl input.json output.LVL` | Converte níveis JSON do Tiled para binário PSX |
| **buildprl** | ✅ Pronto | `buildprl parallax.toml` | Converte config de parallax TOML para PRL |
| **framepacker** | ✅ Pronto | `framepacker [--tilemap] in.json out.CHARA` | Empacota frames de Aseprite |
| **chunkgen** | ✅ Pronto | `chunkgen input.cnk output.MAP` | Converte chunks CSV para MAP |
| **chunkmapper** | ✅ Pronto | `chunkmapper input.json` | Gera TMX a partir de JSON de mapeamento |
| **cookobj** | ⏳ TODO | - | Cozinha objetos Tiled (XML + TOML) |
| **cookcollision** | ✅ Pronto | `cookcollision tiles.json tiles.COL` | Gerado dados de colisão (yyjson) |
| **convrsd** | ⏳ TODO | `convrsd input.rsd` | Converte modelos RSD para MDL |

Legenda: ✅ Pronto | 🚧 Em desenvolvimento | ⏳ A fazer

## Build

```bash
# Build tudo:
make

# Limpar:
make clean
```

Requisitos: GCC/Clang, libc, libm (math)

## Uso

As ferramentas mantêm compatibilidade de CLI com as versões Python:

```bash
# cooklvl - níveis Tiled
./bin/cooklvl assets/levels/R2/level.json assets/levels/R2/level.LVL

# buildprl - parallax (output automático em PRL.PRL no mesmo dir)
./bin/buildprl assets/levels/R2/parallax.toml

# framepacker - sprites/characters
./bin/framepacker assets/sonic.json assets/sonic.CHARA

# framepacker - tilemaps (modo level)
./bin/framepacker --tilemap assets/tiles.json assets/tiles.TILE

# chunkgen - chunks (128x128)
./bin/chunkgen assets/chunks/map128_solid.cnk assets/chunks/map128.MAP

# chunkgen - múltiplas camadas
./bin/chunkgen assets/chunks/map128 assets/chunks/map128.MAP
# Lê map128_solid.cnk, map128_oneway.cnk, map128_none.cnk, map128_front.cnk
```

## Estrutura do Projeto

```
native/
├── Makefile           # Build system
├── README.md          # Este arquivo
├── include/           # Headers públicos
│   ├── psxtypes.h    # Tipos PSX, endianness helpers
│   └── minilib.h     # Utilitários comuns
├── lib/              # Bibliotecas (single-file/embedded)
│   ├── cJSON.h/c     # Parser JSON (cJSON minimal)
│   ├── toml.h        # Parser TOML interface
│   ├── tomlc99.c     # Implementation TOML
│   └── minilib.c     # CSV parsing, file I/O
├── src/              # Código das ferramentas
│   ├── cooklvl.c     # ✅
│   ├── buildprl.c    # ✅
│   ├── framepacker.c # ✅
│   ├── chunkgen.c    # ✅
│   ├── chunkmapper.c # ✅
│   ├── cookcollision.c # ⏳ ver nota abaixo
│   └── convrsd.c     # ⏳
└── bin/              # Binários (gitignored)
```

## Decisões de Design

### Parsing de Formatos
- **JSON**: Implementação minimalista de cJSON (single-file, ~7KB) o suficiente para os arquivos de asset
- **TOML**: Implementação custom minimalista que suporta o subset usado em parallax.toml
- **CSV**: Parser manual para chunks (sem dependências externas)

### Endianness
Todas as ferramentas gerem **big-endian** (formato PlayStation nativo) usando helpers em `psxtypes.h`:
- `write_u16_be()`, `write_u32_be()`, `write_s32_be()`, etc.

### Performance
- Sem dependências externas pesadas (numpy/pandas/shapely)
- Parsing streaming para arquivos grandes
- Alocação mínima de memória

## Ferramentas Pendentes

### cookcollision.c – **Usar Python**
A implementação C está funcional mas o parser JSON minimal não lida bem com arquivos Tiled grandes (>30KB, muitos objetos). **Recomendação**: manter versão Python para collision e fazer wrapper se necessário. Alternativas:
1. Integrar cJSON completo (+100KB código)
2. Implementar parser XML Tiled específico
3. Usar Python (recomendado – funciona)

### cookobj.c (Alta Prioridade)
Substitui `cookcollision.py` que usa shapely para geometria. Precisa implementar:
- Point-in-polygon (ray casting ou winding number)
- Height mask calculation (linecast iterativo)
- Vector normalization
- Cálculo de ângulos em fixed-point (20.12)

### convrsd.c (Alta Prioridade)
Substitui o módulo `convrsd/` de 7 arquivos Python. Complexidade:
- Parser de arquivos RSD/PLY/MAT (formato custom Sony)
- Suporte a múltiplos tipos de faces (tri, quad, line, sprite)
- Múltiplos tipos de material (Flat, Gouraud, Texture, etc.)
- Geração de polígonos PSX format

### cookobj.c (Média Prioridade)
Substitui `cookobj/` (3 arquivos Python):
- Parser XML com BeautifulSoup (Tiled exports)
- Parser TOML para animações
- Geração de OTD (Object Type Definition)
- Geração de OMP (Object Map Placement)

### chunkmapper.c (Baixa Prioridade)
Gera TMX a partir de JSON exportado. Usa pandas em Python; em C:
- Parsing do formato específico de export Aseprite
- Geração de XML (TMX format)

## Comparação de Performance (Python vs C)

| Ferramenta | Python (reps) | C (reps) | Speedup |
|------------|---------------|----------|---------|
| cooklvl | TBD | TBD | Estimado >5x |
| buildprl | TBD | TBD | Estimado >10x |
| framepacker | TBD | TBD | Estimado >5x |
| chunkgen | ~20x* | Parser manual O(n) vs pandas |

*chunkgen: ganho real significativo devido à eliminação do startup do Python + pandas

**Nota**: cookcollision permanece em Python – parser de geometria complexa em C exigiria biblioteca JSON/XML completa que eliminaria vantagem de tamanho.

## Notas de Implementação

### cJSON Minimal
A versão embarcada não suporta:
- Unicode escapes completos (básico apenas)
- Números de ponto flutuante com notação científica
- Arrays/Objetos aninhados extremamente profundos

Para assets de jogo, estas limitações não são problema.

### tomlc99 Minimal
A versão embarcada não suporta:
- Arrays de tabelas
- Inline tables completos
- Strings multiline
- Datetimes

Suficiente para formato de parallax atual.

## Contribuindo

Para adicionar uma nova ferramenta:
1. Criar `src/nome.c` com `main()` compatível com Python
2. Adicionar regra no `Makefile`
3. Documentar no README
4. Testar contra versão Python (output deve ser idêntico)

## Licença

MPL 2.0 (mesma do projeto engine-psx)

## Créditos

- cJSON baseado no trabalho de Dave Gamble (MIT License)
- PSX types baseados em PSn00bSDK
