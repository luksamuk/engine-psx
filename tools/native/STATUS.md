# Status das Ferramentas Native

Data: 2026-04-02

## ✅ Implementadas e Validadas (5/8)

| Ferramenta | Status | CLI | Validação |
|------------|--------|-----|-----------|
| **cooklvl** | ✅ Pronto | `cooklvl input.json output.LVL` | Output binário idêntico |
| **buildprl** | ✅ Pronto | `buildprl parallax.toml` | Output binário idêntico |
| **framepacker** | ✅ Pronto | `framepacker [--tilemap] in.json out.CHAR` | Output binário idêntico |
| **chunkgen** | ✅ Pronto | `chunkgen input.cnk output.MAP` | Output binário idêntico |
| **chunkmapper** | ✅ Pronto | `chunkmapper input.json` | Gera TMX válido |

## 🚧 Em Desenvolvimento

| Ferramenta | Status | Problemas |
|------------|--------|-----------|
| **cookcollision** | 🚧 WIP | cJSON minimal não lida com JSON grandes (>30KB). Precisa de parser mais robusto. |

## ⏳ Pendentes

| Ferramenta | Complexidade | Descrição |
|------------|--------------|-----------|
| **cookobj** | **Alta** | XML + TOML parsing (BeautifulSoup + toml) |
| **convrsd** | **Muito Alta** | Parser RSD/PLY/MAT (formatos customizados) |

## Notas

### cookcollision
A implementação C está completa mas o parser JSON minimal não consegue lidar com os arquivos de export do Tiled (>30KB, objetos complexos). Opções:
1. Usar cJSON completo (adiciona ~100KB)
2. Implementar parser próprio específico para formato Tiled
3. Manter versão Python para collision (mais pesado mas funciona)

### cookobj
Requer:
- Parser XML Tiled (substituir BeautifulSoup)
- Integração com toml existente
- Geração OTD e OMP

### convrsd
Requer:
- Parser formato PLY (Sony SDK)
- Parser formato MAT (materiais)
- Suporte a múltiplos tipos de polígonos (F3, FT3, G3, GT3, etc.)

## Build

```bash
cd tools/native
make

# Todas as ferramentas em tools/native/bin/
```
