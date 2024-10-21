#ifndef PARALLAX_H
#define PARALLAX_H

#include <stdint.h>

/* .PRL file layout
   ==================
   number of strips: u8
   Array of strips:
      number of parts: u8
      do not repeat (single)?: u8
      horizontal scroll factor: s16
      Y position: s16
      Array of parts:
          u0: u8
          v0: u8
          texture index (BG0 or BG1): u8
          width: u16
          height: u16
 */

// Holds a single parallax strip part.
// Parts are a horizontal slice of a single strip.
typedef struct {
    uint8_t  u0;
    uint8_t  v0;
    uint8_t  bgindex;
    uint16_t width;
    uint16_t height;
} ParallaxPart;

// Holds a single parallax strip for a level.
// A strip may be a horizontally-repeating quad, or it
// may be composed of more than one quad that forms a bigger strip.
typedef struct {
    uint8_t num_parts;
    uint8_t is_single;
    int16_t scrollx;
    int16_t y0;
    uint16_t width; // Sum of all widths, calculated on load!
    ParallaxPart *parts;
} ParallaxStrip;

// Holds all parallax strips for a level
typedef struct {
    uint8_t       num_strips;
    ParallaxStrip *strips;
} Parallax;

void load_parallax(Parallax *parallax, const char *filename);

#endif
