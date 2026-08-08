#include <cstdint>

struct Song {
  char song_name[24];
  char song_album[36];
  char song_artist[24];
  uint8_t track_number;
  uint32_t lba_address;
};

struct Disc {
  uint8_t total_tracks;
  uint8_t disc_id;
  struct Song tracks[100];
};
