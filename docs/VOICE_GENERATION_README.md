# FPVGate Voice Generation Guide

## Overview
This guide explains how to generate natural-sounding voice announcements for your FPVGate lap timer using ElevenLabs TTS.

## Features
- ✅ 4 voice options (Sarah, Rachel, Adam, Antoni)
- ✅ Pre-generated audio for top 50 common pilot names
- ✅ 3 lap announcement formats (Pilot+Lap+Time / Lap+Time / Time only)
- ✅ 1.3x faster playback for race announcements
- ✅ 100% offline once generated

## Setup

### 1. Install Python Dependencies
```bash
pip install elevenlabs python-dotenv
```

### 2. Get ElevenLabs API Key
1. Sign up at https://elevenlabs.io/ (FREE tier: 10,000 characters/month)
2. Get your API key from https://elevenlabs.io/app/settings/api-keys
3. Set the environment variable:

**Windows (PowerShell):**
```powershell
$env:ELEVENLABS_API_KEY='your_key_here'
```

**Windows (CMD):**
```cmd
set ELEVENLABS_API_KEY=your_key_here
```

**Linux/Mac:**
```bash
export ELEVENLABS_API_KEY='your_key_here'
```

### 3. Configure Your Settings

Edit `generate_voice_files.py`:

```python
# Personal settings
PILOT_NAME = "Louis"       # Your actual name
PHONETIC_NAME = "Louie"    # How TTS should pronounce it

# Voice selection - choose one:
SELECTED_VOICE = 'default'  # Sarah - Energetic Female (default)
# SELECTED_VOICE = 'rachel'  # Rachel - Calm Female
# SELECTED_VOICE = 'adam'    # Adam - Deep Male
# SELECTED_VOICE = 'antoni'  # Antoni - Well-rounded Male
```

### 4. Generate Audio Files

```bash
python generate_voice_files.py
```

This will generate approximately **125 audio files** (~2.5 MB):
- Race control phrases (5 files)
- Lap numbers 1-50 (50 files)
- Digit numbers 0-9 + "point" (11 files)
- Your personal name (1 file: `yourname_lap.mp3`)
- Top 50 common names (50 files: `alex_lap.mp3`, `sarah_lap.mp3`, etc.)

**Estimated API Usage:** ~1,000 characters (10% of free tier)

### 5. Upload to ESP32

```bash
platformio run -e ESP32S3 -t uploadfs
```

## Using Different Lap Announcement Formats

In the FPVGate web interface (Configuration tab):

### Format Options:
1. **Pilot + Lap + Time** (Default)
   - Example: "Louis Lap 5, 12.34"
   - Best for knowing who's flying and their progress

2. **Lap + Time**
   - Example: "Lap 5, 12.34"
   - Shorter, good for single pilot practice

3. **Time Only**
   - Example: "12.34"
   - Fastest, minimal announcements

## Changing Voices

1. Change `SELECTED_VOICE` in `generate_voice_files.py`
2. Run the script again to regenerate all files
3. Upload filesystem to ESP32
4. Refresh the webpage

### Voice Characteristics:
- **Sarah (default):** Energetic, clear, perfect for race announcements
- **Rachel:** Calm, professional, less excitement
- **Adam:** Deep male voice, authoritative
- **Antoni:** Well-rounded male voice, neutral

## Pre-Generated Names

The following 50 names are automatically generated:

**Male Names:**
Alex, Andrew, Ben, Brandon, Brian, Carlos, Chad, Chris, Daniel, Dave, David, Derek, Eric, Evan, Frank, George, Jack, James, Jason, Jeff, John, Jordan, Josh, Justin, Kevin, Kyle, Lucas, Mark, Matt, Michael, Mike, Nick, Patrick, Paul, Peter, Rob, Ryan, Sam, Scott, Sean, Steve, Thomas, Tim, Tom, Tony, Tyler, Will, Zach

**Female Names:**
Emma, Sarah

**Your name will also be generated** based on `PILOT_NAME` and `PHONETIC_NAME` settings.

### Multi-Pilot Support
If your friends use common names, their announcements will automatically use pre-recorded audio! Just have them:
1. Enter their name in the Configuration tab
2. Use the phonetic name field if needed (e.g., "Jon" → "John")

## Troubleshooting

### "No audio mapping found"
- Make sure the phonetic name matches a generated file
- Check that filesystem was uploaded successfully
- Verify the `.mp3` files exist in `data/sounds/`

### "Audio file not found (404)"
- Re-upload filesystem: `platformio run -e ESP32S3 -t uploadfs`
- Clear browser cache (Ctrl+Shift+R)

### "Comma" or weird sounds
- This was fixed in the latest version
- Re-upload filesystem to get the fix

### API Usage
- Each file costs ~8-15 characters
- 125 files ≈ 1,000 characters
- Free tier: 10,000 characters/month
- **You can regenerate ~10 times per month on free tier!**

## File Structure

```
data/sounds/
├── arm_your_quad.mp3          # Race control
├── starting_tone.mp3
├── race_complete.mp3
├── race_stopped.mp3
├── hole_shot.mp3
├── lap_1.mp3 ... lap_50.mp3   # Lap numbers
├── num_0.mp3 ... num_9.mp3    # Digits
├── point.mp3                   # Decimal point
├── louie_lap.mp3              # Your name
├── alex_lap.mp3               # Common names
├── sarah_lap.mp3
└── ... (50 more names)
```

## Tips

1. **Phonetic Names:** Use the phonetic field for better pronunciation
   - "Louis" → "Louie"
   - "Sean" → "Shawn"
   - "Geoff" → "Jeff"

2. **Battery Life:** Pre-recorded audio uses less CPU than Web Speech API

3. **Offline Racing:** Once uploaded, everything works 100% offline

4. **Voice Speed:** Adjust "Announcer Rate" in Configuration (default: 1.3x for faster announcements)

## Credits

- **ElevenLabs:** Natural TTS voices (https://elevenlabs.io/)
- **FPVGate:** Fork of PhobosLT lap timer
