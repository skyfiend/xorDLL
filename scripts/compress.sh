#!/bin/bash
# Compress executables with UPX
# Creates _packed versions to preserve originals

BUILD_DIR="${1:-build/bin}"
UPX_PATH="${UPX_PATH:-upx}"

# Try to find UPX
if ! command -v "$UPX_PATH" &> /dev/null; then
    # Try common locations
    if [ -x "/tmp/upx-4.2.1-amd64_linux/upx" ]; then
        UPX_PATH="/tmp/upx-4.2.1-amd64_linux/upx"
    else
        echo "UPX not found. Install with: apt install upx-ucl"
        echo "Or download from: https://github.com/upx/upx/releases"
        exit 1
    fi
fi

echo "Using UPX: $UPX_PATH"
echo "Compressing executables..."
echo ""

for file in "$BUILD_DIR"/*.exe "$BUILD_DIR"/*.dll; do
    if [ -f "$file" ] && [[ ! "$file" == *"_packed"* ]]; then
        base="${file%.*}"
        ext="${file##*.}"
        output="${base}_packed.${ext}"
        
        echo "Compressing: $(basename "$file")"
        "$UPX_PATH" --best --lzma -o "$output" "$file" 2>/dev/null || \
        "$UPX_PATH" --best -o "$output" "$file"
        echo ""
    fi
done

echo "=== Results ==="
ls -lh "$BUILD_DIR"/*_packed* 2>/dev/null || echo "No packed files found"
echo ""
echo "Original vs Packed:"
for file in "$BUILD_DIR"/*_packed*; do
    if [ -f "$file" ]; then
        orig="${file/_packed/}"
        if [ -f "$orig" ]; then
            orig_size=$(stat -c%s "$orig")
            pack_size=$(stat -c%s "$file")
            ratio=$((pack_size * 100 / orig_size))
            echo "  $(basename "$orig"): $(numfmt --to=iec $orig_size) -> $(numfmt --to=iec $pack_size) (${ratio}%)"
        fi
    fi
done
