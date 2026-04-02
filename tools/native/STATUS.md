# Status das Ferramentas Native

Data: 2026-04-02

## ✅ Implementadas (4/8)

### cooklvl
- **Substitui**: `tools/cooklvl.py`
- **Dependências eliminadas**: json (stdlib), ctypes
- **Binário**: 22KB (vs Python + overhead)
- **Status**: Funcional, testado com estrutura JSON
- **Formato**: JSON → LVL binário (big-endian)

### buildprl  
- **Substitui**: `tools/buildprl/buildprl.py`
- **Dependências eliminadas**: toml (pypi), dataclasses
- **Binário**: 23KB
- **Status**: Funcional, parser TOML minimal integrado
- **Formato**: TOML → PRL binário (strip de parallax)

### framepacker
- **Substitui**: `tools/framepacker.py`
- **Dependências eliminadas**: json, ctypes
- **Binário**: 26KB
- **Status**: Funcional, suporta modo sprite e tilemap
- **Formato**: JSON (Aseprite export) → CHARA binário

### chunkgen
- **Substitui**: `tools/chunkgen.py`
- **Dependências eliminadas**: pandas, numpy, math
- **Binário**: 22KB
- **Status**: Funcional, CSV parser manual
- **Formato**: CSV(s) → MAP binário (128x128 chunks)

## ⏳ Pendentes

### chunkmapper
- **Substitui**: `tools/chunkmapper.py`
- **Complexidade**: Média
- **Desafios**: 
  - Portar lógica de pandas DataFrame para C arrays
  - Gerar XML válido para TMX

### cookcollision
- **Substitui**: `tools/cookcollision.py`
- **Complexidade**: Alta
- **Desafios principal**:
  - Substituir shapely (geometria) por algoritmos próprios
  - Point-in-polygon (ray casting)
  - Height mask por linecasting
  - Cálculo de ângulos (atan2, normalização)

### cookobj
- **Substitui**: `tools/cookobj/` (cookobj.py, parsers.py, datatypes.py)
- **Complexidade**: Alta
- **Desafios**:
  - Parser XML para Tiled (replaces BeautifulSoup)
  - Parser TOML para definições de objetos
  - Geração OTD (object type definitions)
  - Geração OMP (object map placements)

### convrsd
- **Substitui**: `tools/convrsd/` (7 arquivos Python)
- **Complexidade**: Muito Alta
- **Desafios**:
  - Parser formato RSD (PlayStation SDK)
  - Parser PLY (vertex data)
  - Parser MAT (material data)
  - Converter para formatos POLY_F3, POLY_FT3, etc.

## Benchmarks Preliminares

Teste rápido em arquivo JSON de ~10KB:

```
$ time python3 cooklvl.py test.json out1.LVL
real    0m0.145s

$ time ./bin/cooklvl test.json out2.LVL  
real    0m0.003s
```

**Speedup estimado: ~50x** para pequenos arquivos. Para builds completos com milhares de assets, o ganho será mais significativo devido ao startup time do Python ser eliminado.

## Notas Técnicas

### Parser JSON (cJSON minimal)
- Single-file implementation (~7KB código)
- Suporta: objetos, arrays, strings, números, null, true, false
- Não suporta: unicode escapes complexos, floats científicos
- Suficiente para Tiled JSON exports

### Parser TOML (tomlc99 minimal)
- Implementação custom minimal
- Suporta: seções, key=value (int, float, bool), strings simples
- Não suporta: arrays de tabelas, inline tables, multiline strings
- Suficiente para formato atual de parallax.toml

### Parser CSV (manual)
- Parsing streaming para suportar arquivos grandes
- Sem alocação excessiva
- Suporta valores negativos e células vazias

## Próximos Passos

1. **Testar integração** com Makefile principal do projeto
2. **Implementar cookcollision** - maior ganho (shapely é pesado)
3. **Implementar chunkmapper** - útil para workflow de tiles
4. **Documentar formatos binários** gerados (para portabilidade)

## Decisões de Arquitetura

- **Sem dependências externas**: Todas as libs são single-file ou headers próprios
- **Big-endian por padrão**: PSX é big-endian; todas as tools usam psxtypes.h
- **Compatibilidade 100%**: Output binário deve ser idêntico ao Python
- **Uso de memória**: Mínimo - sem alocação dinâmica excessiva
