#!/usr/bin/env python3
"""
Generate audio files with all 4 voice options
Creates separate folders for each voice
"""

import os
import sys
from pathlib import Path
from generate_voice_files import generate_audio_files, VOICES, PILOT_NAME, PHONETIC_NAME, TOP_NAMES, get_phrases_to_generate

def main():
    """Generate audio for all voices"""
    
    print("\n" + "="*60)
    print("🎤 FPVGate - Generate All Voices")
    print("="*60)
    
    # Check for API key
    api_key = os.getenv("ELEVENLABS_API_KEY")
    
    if not api_key:
        print("\n❌ ERROR: ELEVENLABS_API_KEY not found!\n")
        print("Set environment variable:")
        print("   Windows (PowerShell): $env:ELEVENLABS_API_KEY='your_key_here'")
        return 1
    
    print(f"\n✅ API key found: {api_key[:8]}...{api_key[-4:]}")
    print(f"📝 Personal pilot name: {PILOT_NAME} (pronounced: {PHONETIC_NAME})")
    print(f"👥 Generating audio for {len(TOP_NAMES)} common pilot names")
    print(f"🔢 Numbers 0-59 for natural time announcements")
    print(f"\n🎙️  Generating with ALL 4 voices:")
    for key, voice in VOICES.items():
        print(f"   - {voice['name']}")
    
    # Get total count
    phrases = get_phrases_to_generate()
    total = len(phrases)
    print(f"\n📊 Total files per voice: {total}")
    print(f"   Grand total: {total * 4} files\n")
    
    # Confirm
    response = input("🚀 Generate all voices? (y/n): ")
    if response.lower() != 'y':
        print("Cancelled.")
        return 0
    
    # Generate for each voice
    for voice_key, voice_info in VOICES.items():
        print(f"\n{'='*60}")
        print(f"Generating: {voice_info['name']}")
        print(f"{'='*60}")
        
        # Update voice ID temporarily
        import generate_voice_files
        original_voice = generate_voice_files.SELECTED_VOICE
        original_id = generate_voice_files.VOICE_ID
        original_output = generate_voice_files.OUTPUT_DIR
        
        # Set new voice and output directory
        generate_voice_files.SELECTED_VOICE = voice_key
        generate_voice_files.VOICE_ID = voice_info['id']
        generate_voice_files.OUTPUT_DIR = Path(f"data/sounds_{voice_key}")
        
        try:
            generate_audio_files(api_key)
            print(f"✅ Completed: {voice_info['name']}")
        except Exception as e:
            print(f"❌ Error generating {voice_info['name']}: {e}")
        finally:
            # Restore original settings
            generate_voice_files.SELECTED_VOICE = original_voice
            generate_voice_files.VOICE_ID = original_id
            generate_voice_files.OUTPUT_DIR = original_output
    
    print(f"\n{'='*60}")
    print("✅ All voices generated!")
    print(f"{'='*60}")
    print("\nOutput directories:")
    for voice_key, voice_info in VOICES.items():
        print(f"   data/sounds_{voice_key}/ - {voice_info['name']}")
    print("\nTo use a different voice:")
    print("1. Copy files from data/sounds_<voice>/ to data/sounds/")
    print("2. Upload filesystem: platformio run -e ESP32S3 -t uploadfs")
    
    return 0

if __name__ == "__main__":
    exit(main())
