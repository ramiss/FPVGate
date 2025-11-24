#!/usr/bin/env python3
"""
Parse PlatformIO partition table binary to find SPIFFS address.
PlatformIO partition tables use ESP-IDF format.
"""

import sys
import struct

def parse_partition_table(partition_data):
    """
    Parse ESP-IDF partition table binary format.
    Each entry is 32 bytes:
    - 16 bytes: name (null-terminated)
    - 2 bytes: type (little-endian)
    - 1 byte: subtype
    - 1 byte: reserved
    - 4 bytes: offset (little-endian)
    - 4 bytes: size (little-endian)
    - 4 bytes: flags (little-endian)
    """
    partitions = []
    offset = 0
    
    # Partition table can have up to 95 entries (ESP-IDF limit)
    # But typically much fewer
    while offset + 32 <= len(partition_data):
        # Read name (16 bytes, null-terminated)
        name_bytes = partition_data[offset:offset+16]
        name = name_bytes.split(b'\x00')[0].decode('utf-8', errors='ignore').strip()
        
        # Check for end marker (all zeros or 0xFF)
        if not name or name == '' or (len(name_bytes) > 0 and name_bytes[0] in [0, 0xFF]):
            # Empty entry, but continue scanning
            offset += 32
            continue
        
        # Read type (2 bytes, little-endian)
        type_val = struct.unpack('<H', partition_data[offset+16:offset+18])[0]
        
        # Read subtype (1 byte)
        subtype = partition_data[offset+18]
        
        # Reserved byte at offset+19
        
        # Read offset (4 bytes, little-endian)
        entry_offset = struct.unpack('<I', partition_data[offset+20:offset+24])[0]
        
        # Read size (4 bytes, little-endian)
        entry_size = struct.unpack('<I', partition_data[offset+24:offset+28])[0]
        
        # Read flags (4 bytes, little-endian)
        flags = struct.unpack('<I', partition_data[offset+28:offset+32])[0]
        
        partitions.append({
            'name': name,
            'type': type_val,
            'subtype': subtype,
            'offset': entry_offset,
            'size': entry_size,
            'flags': flags
        })
        
        offset += 32
        
        # Safety limit: don't scan more than 100 entries
        if len(partitions) >= 100:
            break
    
    return partitions

def find_spiffs_partition(partition_data):
    """Find SPIFFS partition and return its address."""
    partitions = parse_partition_table(partition_data)
    
    for part in partitions:
        # Check by subtype: 0x82 = SPIFFS (data subtype)
        # Check by name: "spiffs" (case-insensitive)
        name_lower = part['name'].lower()
        if part['subtype'] == 0x82 or 'spiffs' in name_lower:
            return f"0x{part['offset']:X}"
    
    return None

def main():
    if len(sys.argv) < 2:
        print("Usage: read_partitions.py <partitions.bin>")
        sys.exit(1)
    
    partition_file = sys.argv[1]
    
    try:
        with open(partition_file, 'rb') as f:
            partition_data = f.read()
        
        # Find SPIFFS
        spiffs_addr = find_spiffs_partition(partition_data)
        
        if spiffs_addr:
            print(spiffs_addr)
            return 0
        else:
            # List all partitions for debugging
            partitions = parse_partition_table(partition_data)
            if partitions:
                print("No SPIFFS partition found. Available partitions:", file=sys.stderr)
                for part in partitions:
                    print(f"  {part['name']}: type=0x{part['type']:02X}, subtype=0x{part['subtype']:02X}, offset=0x{part['offset']:X}, size=0x{part['size']:X}", file=sys.stderr)
            else:
                print("No partitions found in table", file=sys.stderr)
            return 1
            
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

if __name__ == '__main__':
    sys.exit(main())

