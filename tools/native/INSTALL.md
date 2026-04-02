# Installation Guide

## Arch Linux

### Minimal Dependencies (required)

```bash
# Build tools
sudo pacman -S base-devel gcc make
```

That's it! All native tools use embedded single-file libraries.

### Verification

```bash
cd tools/native
make clean && make
ls -la bin/
```

### Optional: For Python tools still in use

```bash
# cookobj (XML parsing) - still Python
sudo pacman -S python python-pip
pip install --user beautifulsoup4 toml

# convrsd (complex format) - still Python  
pip install --user typing
```

## Ubuntu/Debian

```bash
sudo apt install build-essential

# Optional Python tools
sudo apt install python3 python3-pip
pip3 install beautifulsoup4 toml
```

## Fedora

```bash
sudo dnf install gcc make glibc-devel

# Optional Python tools
sudo dnf install python3 python3-pip
pip3 install beautifulsoup4 toml
```

## Tool Status

| Tool | Language | Status |
|------|----------|--------|
| cooklvl | C | native ✅ |
| buildprl | C | native ✅ |
| framepacker | C | native ✅ |
| chunkgen | C | native ✅ |
| chunkmapper | C | native ✅ |
| cookcollision | C + yyjson | native ✅ |
| cookobj | Python | needs `beautifulsoup4`, `toml` ⏳ |
| convrsd | Python | needs `typing` ⏳ |

## Notes

- `base-devel` / `build-essential` = gcc, make, libc, binutils
- yyjson is embedded in `tools/native/lib/yyjson/` (single-file)
- No external JSON/TOML/XML libraries required for native tools
