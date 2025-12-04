# FPVGate Tools

This folder contains Python utility scripts for voice generation and SD card management.

## Prerequisites

```bash
pip install -r ../requirements.txt
```

## Voice Generation Scripts

### generate_voice_files.py
Generates individual voice files using ElevenLabs API for a single voice.

**Usage:**
```bash
python generate_voice_files.py
```

**Features:**
- Prompts for ElevenLabs API key
- Generates MP3 files for lap times, numbers, and phrases
- Saves files to `data/sounds_<voice_name>/`

---

### generate_voice_files_auto.py
Automated batch generation for multiple voices without prompts.

**Usage:**
```bash
python generate_voice_files_auto.py
```

**Features:**
- Reads API key from environment or config
- Generates all voices in sequence
- No user interaction required

---

### generate_all_voices.py
Master script to generate all available voices (Sarah, Rachel, Adam, Antoni).

**Usage:**
```bash
python generate_all_voices.py
```

**Features:**
- Generates complete voice packs for all voices
- Creates separate folders for each voice
- Batch processing for faster generation

---

### regenerate_audio.py
Regenerates missing or corrupted audio files for a specific voice.

**Usage:**
```bash
python regenerate_audio.py
```

**Features:**
- Scans existing voice folders
- Identifies missing files
- Regenerates only what's needed

---

### regenerate_audio_auto.py
Automated version of regenerate_audio.py for batch processing.

**Usage:**
```bash
python regenerate_audio_auto.py
```

**Features:**
- Non-interactive regeneration
- Processes all voice folders
- Useful for CI/CD pipelines

---

## SD Card Management

### upload_sounds_to_sd.py
Uploads generated voice files to ESP32-S3 SD card via serial connection.

**Usage:**
```bash
python upload_sounds_to_sd.py
```

**Features:**
- Connects to ESP32-S3 over USB
- Uploads voice files to SD card
- Verifies successful transfer
- Progress indicator for large batches

**Requirements:**
- ESP32-S3 connected via USB
- SD card inserted in ESP32-S3
- Serial drivers installed

---

## Voice File Structure

Generated voice files follow this naming convention:

```
sounds_<voice_name>/
├── gate_1.mp3           # "Gate 1"
├── lap_1.mp3            # "Lap 1"
├── lap_2.mp3            # "Lap 2"
...
├── number_0.mp3         # "zero"
├── number_1.mp3         # "one"
...
├── point.mp3            # "point"
└── seconds.mp3          # "seconds"
```

## Notes

- Voice generation requires an active ElevenLabs API subscription
- Generated files are approximately 1-2MB per voice pack
- SD card upload requires ~5-10 minutes for a full voice pack
- All scripts support both Windows and Linux

## Troubleshooting

**Issue**: "Module not found" error  
**Solution**: Install requirements with `pip install -r ../requirements.txt`

**Issue**: SD upload fails  
**Solution**: Check ESP32-S3 is connected and SD card is formatted as FAT32

**Issue**: Voice generation timeout  
**Solution**: Check your ElevenLabs API key and internet connection

## See Also

- [Voice Generation Guide](../docs/VOICE_GENERATION_README.md)
- [Multi-Voice Setup](../docs/MULTI_VOICE_SETUP.md)
- [SD Card Migration Guide](../docs/SD_CARD_MIGRATION_GUIDE.md)
