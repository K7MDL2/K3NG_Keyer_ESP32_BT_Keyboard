#ifndef KEYER_MORSE_H
#define KEYER_MORSE_H

struct MorseLookupOptions {
  int prosign_start;  // Zero disables prosigns; otherwise the existing PROSIGN_START.
  bool non_english;
  bool keyboard_lcd;
};

// Decimal digits 1/2 encode dits/dahs; 9 is a space. -1 means unknown.
int keyer_morse_character(long pattern, const MorseLookupOptions &options);

#endif
