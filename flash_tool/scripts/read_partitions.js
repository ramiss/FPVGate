#!/usr/bin/env node
/**
 * Read SPIFFS partition address from partitions.bin file
 * PlatformIO partition table format parser
 */

const fs = require('fs');

// Partition table entry structure (from ESP-IDF)
// Each entry is 32 bytes:
// - 16 bytes: name (null-terminated)
// - 2 bytes: type
// - 1 byte: subtype
// - 4 bytes: offset (little-endian)
// - 4 bytes: size (little-endian)
// - 1 byte: flags
// - 4 bytes: reserved

function readPartitionTable(partitionsPath) {
    if (!fs.existsSync(partitionsPath)) {
        throw new Error(`Partition file not found: ${partitionsPath}`);
    }
    
    const data = fs.readFileSync(partitionsPath);
    
    // Partition table starts at offset 0
    // First entry is at offset 0, each entry is 32 bytes
    const entries = [];
    let offset = 0;
    
    while (offset + 32 <= data.length) {
        // Read entry
        const nameBytes = data.slice(offset, offset + 16);
        const name = nameBytes.toString('utf8').split('\0')[0].trim();
        
        // Check for end marker (empty name or all zeros)
        if (!name || name.length === 0) {
            break;
        }
        
        const type = data.readUInt16LE(offset + 16);
        const subtype = data.readUInt8(offset + 18);
        const entryOffset = data.readUInt32LE(offset + 20);
        const size = data.readUInt32LE(offset + 24);
        
        entries.push({
            name,
            type,
            subtype,
            offset: `0x${entryOffset.toString(16)}`,
            size: `0x${size.toString(16)}`
        });
        
        offset += 32;
    }
    
    return entries;
}

function findSPIFFSPartition(partitionsPath) {
    const entries = readPartitionTable(partitionsPath);
    
    // Find SPIFFS partition (subtype 0x82 for SPIFFS)
    const spiffs = entries.find(e => 
        e.name.toLowerCase() === 'spiffs' || 
        e.subtype === 0x82 ||
        (e.type === 1 && e.name.toLowerCase().includes('spiffs')) // Type 1 = data, subtype 0x82 = spiffs
    );
    
    return spiffs;
}

// CLI usage
if (require.main === module) {
    const args = process.argv.slice(2);
    if (args.length === 0) {
        console.error('Usage: node read_partitions.js <partitions.bin>');
        process.exit(1);
    }
    
    try {
        const spiffs = findSPIFFSPartition(args[0]);
        if (spiffs) {
            console.log(JSON.stringify({
                address: spiffs.offset,
                size: spiffs.size,
                name: spiffs.name
            }));
        } else {
            console.error('SPIFFS partition not found in partition table');
            console.log('Available partitions:');
            const entries = readPartitionTable(args[0]);
            entries.forEach(e => {
                console.log(`  ${e.name}: ${e.offset} (size: ${e.size})`);
            });
            process.exit(1);
        }
    } catch (error) {
        console.error(`Error: ${error.message}`);
        process.exit(1);
    }
}

module.exports = { readPartitionTable, findSPIFFSPartition };

