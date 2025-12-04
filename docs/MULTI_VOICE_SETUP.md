# Multi-Voice Setup Guide

FPVGate now includes **4 pre-generated ElevenLabs voices** stored on SD card for instant voice switching!

## Available Voices

| Voice ID | Name | Description | Size |
|----------|------|-------------|------|
| `default` | Sarah | Energetic Female | 3.05 MB |
| `rachel` | Rachel | Calm Female | 3.17 MB |
| `adam` | Adam | Deep Male | 3.05 MB |
| `antoni` | Antoni | Well-rounded Male | 3.17 MB |
| `piper` | PiperTTS | Synthetic (lower latency) | No files |

**Total storage required:** ~12.5 MB (all voices combined)

## SD Card Setup

### Directory Structure
Copy all voice directories to your SD card root:

```
SD:\
├── sounds_default\  (Sarah - 214 files)
│   ├── arm_your_quad.mp3
│   ├── lap_1.mp3
│   └── ... (all MP3 files)
├── sounds_rachel\   (Rachel - 214 files)
├── sounds_adam\     (Adam - 214 files)
└── sounds_antoni\   (Antoni - 214 files)
```

### Quick Copy (Windows PowerShell)
```powershell
# From FPVGate project directory
$sdDrive = "E:"  # Change to your SD card drive letter
Copy-Item -Recurse "data\sounds_default" "$sdDrive\"
Copy-Item -Recurse "data\sounds_rachel" "$sdDrive\"
Copy-Item -Recurse "data\sounds_adam" "$sdDrive\"
Copy-Item -Recurse "data\sounds_antoni" "$sdDrive\"
```

## Changing Voices

### Via Web Interface
1. Connect to FPVGate WiFi
2. Open web interface (http://192.168.4.1)
3. Go to **Configuration** tab
4. Under **TTS Settings**, find **Voice** dropdown
5. Select your preferred voice:
   - ElevenLabs (Sarah - Energetic)
   - ElevenLabs (Rachel - Calm)
   - ElevenLabs (Adam - Deep Male)
   - ElevenLabs (Antoni - Male)
   - PiperTTS (Lower Latency)
6. Click "Test Voice" to hear the selected voice
7. Settings auto-save immediately

### Via Browser Console
```javascript
// Change voice directly
audioAnnouncer.setVoice('rachel');  // or 'default', 'adam', 'antoni', 'piper'

// Test the new voice
audioAnnouncer.queueSpeak('<p>Testing new voice</p>');
```

## Voice Characteristics

### Sarah (default)
- **Tone:** Energetic, excited
- **Best for:** Race announcements, high-energy sessions
- **Speed:** Fast, clear pronunciation
- **Use when:** You want motivating, upbeat callouts

### Rachel
- **Tone:** Calm, professional
- **Best for:** Training sessions, analysis
- **Speed:** Measured, deliberate
- **Use when:** You want clear, relaxed announcements

### Adam
- **Tone:** Deep, authoritative
- **Best for:** Serious racing, competition
- **Speed:** Commanding presence
- **Use when:** You want a male voice with authority

### Antoni
- **Tone:** Well-rounded, neutral male
- **Best for:** All-purpose use
- **Speed:** Balanced delivery
- **Use when:** You want a versatile male voice

### PiperTTS
- **Tone:** Synthetic but clear
- **Best for:** Lower latency, offline-only
- **Speed:** Instant generation
- **Use when:** SD card unavailable or you need fastest response

## How It Works

### Audio File Priority
1. **Check SD card** for selected voice directory
2. **Load MP3 file** if exists
3. **Fallback to PiperTTS** if file missing
4. **Final fallback** to Web Speech API

### Voice Selection Flow
```
User selects "Rachel" → 
  localStorage saves preference →
    AudioAnnouncer loads from sounds_rachel/ →
      Streams audio directly from SD card
```

### Caching
- Audio files cached in browser memory after first play
- Cache cleared when changing voices
- Instant playback on subsequent uses

## Storage Requirements

### SD Card
- **All 4 voices:** 12.5 MB
- **Single voice:** ~3 MB
- **Recommended:** Copy all voices for flexibility

### ESP32 Flash (LittleFS)
- **Web interface only:** ~300 KB
- **No audio files:** All on SD card
- **85% space savings** vs previous version

## Troubleshooting

### Voice Not Playing
1. **Check SD card** - Verify voice directory exists
2. **Check directory name** - Must be exact: `sounds_default`, `sounds_rachel`, etc.
3. **Check file count** - Each voice should have 214 MP3 files
4. **Check serial monitor** - Look for "Serving audio from SD: sounds_xxx/..."

### Voice Sounds Wrong
- **Cache issue:** Change voice away and back, or reload page
- **Wrong directory:** Check localStorage: `localStorage.getItem('selectedVoice')`
- **Missing files:** Some files may be missing, fallback to TTS

### Switching Doesn't Work
1. Open browser console (F12)
2. Run: `localStorage.setItem('selectedVoice', 'rachel')`
3. Reload page
4. Test voice

### SD Card Not Detected
- Voice selection will still work
- All voices fallback to PiperTTS
- No pre-recorded audio available

## Performance

### Latency Comparison
| Voice Type | First Play | Subsequent | Quality |
|------------|-----------|------------|---------|
| ElevenLabs (SD) | ~100ms | ~10ms | Excellent |
| PiperTTS | ~500ms | ~500ms | Good |
| Web Speech | ~200ms | ~200ms | Varies |

### Memory Usage
- **Per voice cache:** ~3 MB RAM (after all files played)
- **Cache clears:** On voice change
- **No impact:** On race timing or RSSI detection

## Advanced: Custom Voices

Want to add your own voice?

1. **Generate audio files** using `generate_voice_files.py`
2. **Create directory** on SD: `sounds_custom/`
3. **Copy files** to directory
4. **Add to code:**
   ```javascript
   // In audio-announcer.js constructor:
   this.voiceDirectories = {
       ...
       'custom': 'sounds_custom'
   };
   ```
5. **Rebuild & upload** web interface

## Tips

- **Test all voices** before a race to find your favorite
- **Sarah** is best for excitement and energy
- **Rachel** is best for calm practice sessions
- **Adam/Antoni** are great for variety
- **PiperTTS** is backup when SD fails
- **Switch anytime** - no restart needed
- **All voices work offline** once on SD card

## Storage Math

```
Old system (LittleFS only):
- 1 voice: 1.69 MB
- Flash limit: Hit at 4.4 MB
- Voices possible: 1

New system (SD card):
- 4 voices: 12.5 MB
- SD capacity: 32 GB
- Voices possible: Unlimited!
- Flash freed: 85%
```

## Future Possibilities

With SD card storage, you can now:
- ✅ Add all 4 ElevenLabs voices
- ✅ Switch voices instantly
- 🔜 Add custom pilot names in all voices
- 🔜 Add multiple languages
- 🔜 Add sound effects (engine sounds, cheering, etc.)
- 🔜 Store race telemetry and RSSI logs

---

**Enjoy your multi-voice FPVGate experience!** 🎉
