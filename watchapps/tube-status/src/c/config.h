#pragma once

#include <pebble.h>

// Persist storage keys
#define PERSIST_KEY_PINNED_LINES 1

// Order is very important - must match JS side
typedef enum {
  LineTypeBakerloo = 0,
  LineTypeCentral,
  LineTypeCircle,
  LineTypeDistrict,
  LineTypeDLR,
  LineTypeElizabeth,
  LineTypeHammersmithAndCity,
  LineTypeJubilee,
  LineTypeLiberty,
  LineTypeLioness,
  LineTypeMetropolitan,
  LineTypeMildmay,
  LineTypeNorthern,
  LineTypePicadilly,
  LineTypeSuffragette,
  LineTypeVictoria,
  LineTypeWaterlooAndCity,
  LineTypeWeaver,
  LineTypeWindrush,

  LineTypeMax
} LineType;
