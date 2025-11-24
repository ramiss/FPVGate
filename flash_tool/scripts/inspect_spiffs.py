#!/usr/bin/env python3
"""
SPIFFS Image Inspector
Extracts and lists contents of a SPIFFS .bin file
"""

import sys
import os
import struct
import argparse

# SPIFFS constants (from PlatformIO/ESP32 defaults)
SPIFFS_MAGIC = 0x20090315
SPIFFS_PAGE_SIZE = 256
SPIFFS_BLOCK_SIZE = 4096

def read_spiffs_header(data, offset):
    """Read SPIFFS object header"""
    if offset + 32 > len(data):
        return None
    
    # SPIFFS object header structure
    magic = struct.unpack('<I', data[offset:offset+4])[0]
    if magic != SPIFFS_MAGIC:
        return None
    
    obj_type = struct.unpack('<B', data[offset+4:offset+5])[0]
    obj_id = struct.unpack('<H', data[offset+5:offset+7])[0]
    size = struct.unpack('<I', data[offset+8:offset+12])[0]
    name_len = struct.unpack('<B', data[offset+12:offset+13])[0]
    
    # Read filename
    name_start = offset + 32
    name_end = name_start + name_len
    if name_end > len(data):
        return None
    
    filename = data[name_start:name_end].decode('utf-8', errors='ignore')
    
    return {
        'type': obj_type,
        'id': obj_id,
        'size': size,
        'name': filename,
        'data_offset': name_end
    }

def extract_by_signature(data, extract_dir):
    """Extract files by searching for content signatures"""
    print("\nExtracting files by content signatures...")
    print("=" * 60)
    
    # Ensure extract directory exists
    os.makedirs(extract_dir, exist_ok=True)
    
    files_extracted = []
    
    # Try to find index.html
    html_markers = [b'<!DOCTYPE html', b'<html', b'<HTML']
    for marker in html_markers:
        idx = data.find(marker)
        if idx != -1:
            # Find the end (look for </html> or end of non-zero data)
            end_idx = data.find(b'</html>', idx)
            if end_idx == -1:
                # Find end of non-zero block (look for long sequence of zeros or 0xFF)
                end_idx = idx
                consecutive_empty = 0
                while end_idx < len(data):
                    byte = data[end_idx]
                    if byte == 0 or byte == 0xFF:
                        consecutive_empty += 1
                        if consecutive_empty > 256:  # End of file
                            break
                    else:
                        consecutive_empty = 0
                    end_idx += 1
                end_idx -= consecutive_empty
            else:
                end_idx += 7  # Include </html>
            
            content = data[idx:end_idx]
            if len(content) > 50:  # Minimum reasonable HTML size
                filepath = os.path.join(extract_dir, 'index.html')
                with open(filepath, 'wb') as f:
                    f.write(content)
                print(f"  Extracted: index.html ({len(content)} bytes) at offset 0x{idx:08x}")
                files_extracted.append(('index.html', len(content)))
                break
    
    # Try to find style.css
    css_markers = [b'body {', b'body{', b'/*', b'@media']
    for marker in css_markers:
        idx = data.find(marker)
        if idx != -1 and idx > 100:  # Avoid false positives at start
            # Find the end (look for end of non-zero CSS block)
            end_idx = idx
            consecutive_empty = 0
            while end_idx < len(data):
                byte = data[end_idx]
                if byte == 0 or byte == 0xFF:
                    consecutive_empty += 1
                    if consecutive_empty > 256:  # End of file
                        break
                else:
                    consecutive_empty = 0
                end_idx += 1
            
            content = data[idx:end_idx-consecutive_empty]
            if len(content) > 50:  # Minimum reasonable CSS size
                filepath = os.path.join(extract_dir, 'style.css')
                with open(filepath, 'wb') as f:
                    f.write(content)
                print(f"  Extracted: style.css ({len(content)} bytes) at offset 0x{idx:08x}")
                files_extracted.append(('style.css', len(content)))
                break
    
    # Try to find app.js
    js_markers = [b'function', b'const ', b'let ', b'var ', b'// JavaScript']
    for marker in js_markers:
        idx = data.find(marker)
        if idx != -1 and idx > 100:  # Avoid false positives
            # Find the end
            end_idx = idx
            consecutive_empty = 0
            while end_idx < len(data):
                byte = data[end_idx]
                if byte == 0 or byte == 0xFF:
                    consecutive_empty += 1
                    if consecutive_empty > 256:
                        break
                else:
                    consecutive_empty = 0
                end_idx += 1
            
            content = data[idx:end_idx-consecutive_empty]
            if len(content) > 50:  # Minimum reasonable JS size
                filepath = os.path.join(extract_dir, 'app.js')
                with open(filepath, 'wb') as f:
                    f.write(content)
                print(f"  Extracted: app.js ({len(content)} bytes) at offset 0x{idx:08x}")
                files_extracted.append(('app.js', len(content)))
                break
    
    return files_extracted

def inspect_spiffs(spiffs_path, extract_dir=None):
    """Inspect SPIFFS image and optionally extract files"""
    if not os.path.exists(spiffs_path):
        print(f"Error: File not found: {spiffs_path}")
        return False
    
    print(f"Reading SPIFFS image: {spiffs_path}")
    
    with open(spiffs_path, 'rb') as f:
        data = f.read()
    
    file_size = len(data)
    print(f"Image size: {file_size} bytes ({file_size / 1024:.1f} KB)")
    print(f"Image size: {file_size / (1024*1024):.2f} MB")
    print()
    
    # Check if file is all zeros (empty/corrupted)
    if all(b == 0 for b in data[:min(1024, len(data))]):
        print("⚠️  WARNING: File appears to be empty (all zeros)")
        print("   This indicates the SPIFFS image is corrupted or was never written.")
        return False
    
    # Try to find SPIFFS objects with different parameters
    print("Scanning for SPIFFS files...")
    print("=" * 60)
    
    files_found = []
    
    # Try different page sizes
    page_sizes = [256, 512, 1024, 4096]
    for page_size in page_sizes:
        offset = 0
        while offset < len(data) - 32:
            # Look for SPIFFS magic
            if offset + 4 <= len(data):
                magic = struct.unpack('<I', data[offset:offset+4])[0]
                if magic == SPIFFS_MAGIC:
                    header = read_spiffs_header(data, offset)
                    if header and header['name']:
                        files_found.append({
                            'offset': offset,
                            'header': header,
                            'page_size': page_size
                        })
                        print(f"Found: {header['name']} (page_size={page_size})")
                        print(f"  Type: {header['type']}")
                        print(f"  Size: {header['size']} bytes")
                        print(f"  Offset: 0x{offset:08x}")
                        print()
            
            # Skip ahead
            offset += page_size
        
        if files_found:
            break
    
    if not files_found:
        print("No SPIFFS files found in image.")
        print()
        print("This could mean:")
        print("  1. The SPIFFS image is empty")
        print("  2. The image uses different page/block sizes")
        print("  3. The image format is different (LittleFS, etc.)")
        print()
        print("Trying alternative method: checking for common file signatures...")
        
        # Check for common file signatures
        if b'<!DOCTYPE html' in data or b'<html' in data:
            print("  ✓ Found HTML content (likely index.html)")
        if b'body {' in data or b'.css' in data:
            print("  ✓ Found CSS content (likely style.css)")
        if b'function' in data or b'const ' in data:
            print("  ✓ Found JavaScript content (likely app.js)")
        
        # Show first 512 bytes as hex dump
        print()
        print("First 512 bytes (hex dump):")
        for i in range(0, min(512, len(data)), 16):
            hex_str = ' '.join(f'{b:02x}' for b in data[i:i+16])
            ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in data[i:i+16])
            print(f"{i:04x}: {hex_str:<48} {ascii_str}")
        
        # Try extracting by signature if extract_dir is specified
        if extract_dir:
            files_extracted = extract_by_signature(data, extract_dir)
            if files_extracted:
                print(f"\n✓ Extracted {len(files_extracted)} files by content signature")
                return True
        
        return False
    
    print(f"Total files found: {len(files_found)}")
    print("=" * 60)
    
    # Extract files if requested
    if extract_dir:
        os.makedirs(extract_dir, exist_ok=True)
        print(f"\nExtracting files to: {extract_dir}")
        
        if files_found:
            # Extract using SPIFFS headers
            for file_info in files_found:
                header = file_info['header']
                filepath = os.path.join(extract_dir, header['name'].lstrip('/'))
                
                # Create directory if needed
                os.makedirs(os.path.dirname(filepath) if os.path.dirname(filepath) else extract_dir, exist_ok=True)
                
                # Extract file data
                data_start = file_info['data_offset']
                data_end = data_start + header['size']
                
                if data_end <= len(data):
                    file_data = data[data_start:data_end]
                    with open(filepath, 'wb') as f:
                        f.write(file_data)
                    print(f"  Extracted: {header['name']} ({len(file_data)} bytes)")
                else:
                    print(f"  ⚠️  Error extracting {header['name']}: data extends beyond image")
        else:
            # Fall back to signature-based extraction
            files_extracted = extract_by_signature(data, extract_dir)
            if not files_extracted:
                print("  ⚠️  Could not extract files (no signatures found)")
        
        print(f"\n✓ Extraction complete!")
    
    return True

def main():
    parser = argparse.ArgumentParser(
        description='Inspect and extract contents from SPIFFS .bin files'
    )
    parser.add_argument('spiffs_file', help='Path to spiffs.bin file')
    parser.add_argument('-e', '--extract', metavar='DIR', 
                       help='Extract files to directory')
    
    args = parser.parse_args()
    
    success = inspect_spiffs(args.spiffs_file, args.extract)
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()

