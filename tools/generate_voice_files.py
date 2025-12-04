#!/usr/bin/env python3
"""
FPVGate Voice File Generator using ElevenLabs API

This script generates all necessary audio files for FPVGate's voice announcements
using ElevenLabs' high-quality neural TTS.

Requirements:
    pip install elevenlabs python-dotenv

Setup:
    1. Sign up for ElevenLabs: https://elevenlabs.io/
    2. Get your API key from: https://elevenlabs.io/app/settings/api-keys
    3. Create a .env file with: ELEVENLABS_API_KEY=your_key_here
    4. Run: python generate_voice_files.py

The free tier gives you 10,000 characters/month which is plenty for this!
"""

import os
import sys
from pathlib import Path
from typing import List, Dict
from elevenlabs import VoiceSettings
from elevenlabs.client import ElevenLabs

# Configuration
OUTPUT_DIR = Path("data/sounds")
PILOT_NAME = "Louis"  # Change this to your name for personalized files
PHONETIC_NAME = "Louie"  # How you want it pronounced

# Voice options - select one below or use VOICE_ID directly
VOICES = {
    'default': {  # Sarah - Energetic female
        'id': 'EXAVITQu4vr4xnSDxMaL',
        'name': 'Sarah (Energetic Female)'
    },
    'rachel': {  # Rachel - Calm female
        'id': '21m00Tcm4TlvDq8ikWAM',
        'name': 'Rachel (Calm Female)'
    },
    'adam': {  # Adam - Deep male
        'id': 'pNInz6obpgDQGcFmaJgB',
        'name': 'Adam (Deep Male)'
    },
    'antoni': {  # Antoni - Well-rounded male
        'id': 'ErXwobaYiN019PkySvjV',
        'name': 'Antoni (Male)'
    }
}

# Select voice - change 'default' to 'rachel', 'adam', or 'antoni'
SELECTED_VOICE = 'default'
VOICE_ID = VOICES[SELECTED_VOICE]['id']

VOICE_SETTINGS = VoiceSettings(
    stability=0.5,        # Lower = more expressive, Higher = more stable
    similarity_boost=0.75, # How close to the original voice
    style=0.0,            # Style exaggeration (0 to 1)
    use_speaker_boost=True
)

# Top 50 most common FPV pilot names (first names)
TOP_NAMES = [
    "Alex", "Andrew", "Ben", "Brandon", "Brian", "Carlos", "Chad", "Chris", 
    "Daniel", "Dave", "David", "Derek", "Eric", "Evan", "Frank", "George",
    "Jack", "James", "Jason", "Jeff", "John", "Jordan", "Josh", "Justin",
    "Kevin", "Kyle", "Lucas", "Mark", "Matt", "Michael", "Mike", "Nick",
    "Patrick", "Paul", "Peter", "Rob", "Ryan", "Sam", "Scott", "Sean",
    "Steve", "Thomas", "Tim", "Tom", "Tony", "Tyler", "Will", "Zach",
    "Emma", "Sarah"  # Including some common female names
]

def get_phrases_to_generate() -> Dict[str, str]:
    """Returns all phrases that need to be generated as {filename: text}"""
    
    phrases = {
        # Race control phrases
        "arm_your_quad.mp3": "Arm your quad",
        "starting_tone.mp3": "Starting on the tone in less than five",
        "race_complete.mp3": "Race complete",
        "race_stopped.mp3": "Race stopped",
        "gate_1.mp3": "Gate 1",
        
        # Test voice phrase
        f"test_sound_{PILOT_NAME.lower()}.mp3": f"Testing sound for pilot {PHONETIC_NAME}",
        
        # Lap phrases
        "lap.mp3": "Lap",
        "laps.mp3": "laps",
        "two_laps.mp3": "2 laps",
        "three_laps.mp3": "3 laps",
        
        # Your personal pilot name (using phonetic pronunciation)
        f"{PILOT_NAME.lower()}_lap.mp3": f"{PHONETIC_NAME} lap",
        f"{PILOT_NAME.lower()}_2laps.mp3": f"{PHONETIC_NAME} 2 laps",
        f"{PILOT_NAME.lower()}_3laps.mp3": f"{PHONETIC_NAME} 3 laps",
        
        # Numbers 0-99 (for natural time announcements)
        **{f"num_{i}.mp3": str(i) for i in range(100)},
        
        # Punctuation
        "point.mp3": "point",
        
        # Ordinal numbers for laps 1-50
        **{f"lap_{i}.mp3": f"Lap {i}" for i in range(1, 51)},
        
        # Top 50 common pilot names (for multi-pilot support)
        **{f"{name.lower()}_lap.mp3": f"{name} lap" for name in TOP_NAMES},
    }
    
    return phrases


def generate_audio_files(api_key: str):
    """Generate all audio files using ElevenLabs API"""
    
    # Initialize client
    client = ElevenLabs(api_key=api_key)
    
    # Create output directory
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    
    # Get all phrases to generate
    phrases = get_phrases_to_generate()
    
    total = len(phrases)
    voice_name = VOICES[SELECTED_VOICE]['name']
    print(f"\n🎤 Generating {total} audio files with ElevenLabs...")
    print(f"   Voice: {voice_name} ({VOICE_ID})")
    print(f"   Output: {OUTPUT_DIR}\n")
    
    success_count = 0
    error_count = 0
    
    for i, (filename, text) in enumerate(phrases.items(), 1):
        output_path = OUTPUT_DIR / filename
        
        # Skip if already exists
        if output_path.exists():
            print(f"[{i}/{total}] ⏭️  Skipping (exists): {filename}")
            success_count += 1
            continue
        
        try:
            print(f"[{i}/{total}] 🔊 Generating: {filename} -> \"{text}\"")
            
            # Generate audio
            audio_generator = client.text_to_speech.convert(
                voice_id=VOICE_ID,
                optimize_streaming_latency="0",
                output_format="mp3_44100_128",
                text=text,
                model_id="eleven_turbo_v2_5",  # Fast and high quality
                voice_settings=VOICE_SETTINGS,
            )
            
            # Save to file
            with open(output_path, "wb") as f:
                for chunk in audio_generator:
                    if chunk:
                        f.write(chunk)
            
            print(f"          ✅ Success: {output_path.stat().st_size // 1024}KB")
            success_count += 1
            
        except Exception as e:
            print(f"          ❌ Error: {e}")
            error_count += 1
    
    # Summary
    print(f"\n{'='*60}")
    print(f"✅ Successfully generated: {success_count}/{total}")
    if error_count > 0:
        print(f"❌ Errors: {error_count}")
    
    total_size = sum(f.stat().st_size for f in OUTPUT_DIR.glob("*.mp3"))
    print(f"📦 Total size: {total_size / 1024 / 1024:.2f} MB")
    print(f"📁 Location: {OUTPUT_DIR.absolute()}")
    print(f"{'='*60}\n")
    
    # Estimate character usage
    total_chars = sum(len(text) for text in phrases.values())
    print(f"📊 ElevenLabs API Usage:")
    print(f"   Characters used: ~{total_chars:,}")
    print(f"   Free tier limit: 10,000/month")
    print(f"   Remaining: ~{10000 - total_chars:,}\n")


def main():
    """Main entry point"""
    
    print("\n" + "="*60)
    print("🎤 FPVGate Voice File Generator (ElevenLabs)")
    print("="*60)
    
    # Check for API key
    api_key = os.getenv("ELEVENLABS_API_KEY")
    
    if not api_key:
        print("\n❌ ERROR: ELEVENLABS_API_KEY not found!\n")
        print("Setup instructions:")
        print("1. Sign up at: https://elevenlabs.io/")
        print("2. Get API key at: https://elevenlabs.io/app/settings/api-keys")
        print("3. Set environment variable:")
        print("   Windows (PowerShell): $env:ELEVENLABS_API_KEY='your_key_here'")
        print("   Windows (CMD): set ELEVENLABS_API_KEY=your_key_here")
        print("   Linux/Mac: export ELEVENLABS_API_KEY='your_key_here'")
        print("\n   OR create a .env file with:")
        print("   ELEVENLABS_API_KEY=your_key_here")
        print()
        sys.exit(1)
    
    print(f"\n✅ API key found: {api_key[:8]}...{api_key[-4:]}")
    print(f"📝 Personal pilot name: {PILOT_NAME} (pronounced: {PHONETIC_NAME})")
    print(f"🎙️ Selected voice: {VOICES[SELECTED_VOICE]['name']}")
    print(f"👥 Generating audio for {len(TOP_NAMES)} common pilot names")
    
    # Confirm before generating
    response = input("\n🚀 Ready to generate audio files? (y/n): ")
    if response.lower() != 'y':
        print("Cancelled.")
        return
    
    try:
        generate_audio_files(api_key)
        print("✅ Done! Your audio files are ready to use.")
        print("\nNext steps:")
        print("1. Review the audio files in data/sounds/")
        print("2. Upload filesystem to ESP32: pio run -e ESP32S3 -t uploadfs")
        print("3. Test the new voice system in FPVGate!")
        
    except Exception as e:
        print(f"\n❌ Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    try:
        from dotenv import load_dotenv
        load_dotenv()  # Load .env file if exists
    except ImportError:
        pass  # dotenv is optional
    
    main()
