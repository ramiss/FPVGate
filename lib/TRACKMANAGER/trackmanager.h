#ifndef TRACKMANAGER_H
#define TRACKMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "storage.h"

#define MAX_TRACKS 50
#define TRACKS_DIR "/tracks"
#define TRACK_IMAGES_DIR "/tracks/images"

struct Track {
    uint32_t trackId;           // Timestamp-based unique ID
    String name;                // Track name (max 50 chars)
    String tags;                // Comma-separated tags (max 100 chars)
    float distance;             // Track distance in meters
    String notes;               // Track notes (max 200 chars)
    String imagePath;           // Path to track image (optional)
};

class TrackManager {
   public:
    TrackManager();
    bool init(Storage* storage);
    bool createTrack(const Track& track);
    bool loadTracks();
    bool deleteTrack(uint32_t trackId);
    bool updateTrack(uint32_t trackId, const Track& updatedTrack);
    bool clearAll();
    String toJsonString();
    Track* getTrackById(uint32_t trackId);
    const std::vector<Track>& getTracks() const { return tracks; }
    size_t getTrackCount() const { return tracks.size(); }
    
    // Image handling
    bool saveTrackImage(uint32_t trackId, const uint8_t* imageData, size_t imageSize);
    bool deleteTrackImage(uint32_t trackId);
    String getTrackImagePath(uint32_t trackId);

   private:
    std::vector<Track> tracks;
    Storage* storage;
    String generateFilename(uint32_t trackId);
};

#endif
