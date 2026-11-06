#include "keyer.h"
#include "keyer_morse.h"
#include <cassert>
#include <climits>
#include <cstdio>

int convert_cw_number_to_ascii(long pattern);
extern int error_tones;

struct Entry { long pattern; int character; };

static int expected(long pattern) {
  const Entry common[] = {
      {12,'A'}, {2111,'B'}, {2121,'C'}, {211,'D'}, {1,'E'}, {1121,'F'},
      {221,'G'}, {1111,'H'}, {11,'I'}, {1222,'J'}, {212,'K'}, {1211,'L'},
      {22,'M'}, {21,'N'}, {222,'O'}, {1221,'P'}, {2212,'Q'}, {121,'R'},
      {111,'S'}, {2,'T'}, {112,'U'}, {1112,'V'}, {122,'W'}, {2112,'X'},
      {2122,'Y'}, {2211,'Z'}, {22222,'0'}, {12222,'1'}, {11222,'2'},
      {11122,'3'}, {11112,'4'}, {11111,'5'}, {21111,'6'}, {22111,'7'},
      {22211,'8'}, {22221,'9'}, {112211,'?'}, {21121,'/'}, {221122,','},
      {121212,'.'}, {122121,'@'}, {222222,'\\'}, {21112,'='}, {211112,'-'}, {9,' '}
  };
  for (const auto &entry : common) if (pattern == entry.pattern) return entry.character;
#ifdef OPTION_PROSIGN_SUPPORT
  const Entry prosigns[] = {
      {12111,PROSIGN_AS}, {2111212,PROSIGN_BK}, {21211211,PROSIGN_CL},
      {21212,PROSIGN_CT}, {21221,PROSIGN_KN}, {211222,PROSIGN_NJ},
      {111212,PROSIGN_SK}, {11121,PROSIGN_SN}, {11111111,PROSIGN_HH}
  };
  for (const auto &entry : prosigns) if (pattern == entry.pattern) return entry.character;
  #ifndef OPTION_NON_ENGLISH_EXTENSIONS
    if (pattern == 1212) return PROSIGN_AA;
  #endif
#else
  if (pattern == 2111212) return '*';
  if (pattern == 21221) return '(';
#endif
#ifdef OPTION_KEYBOARD_NON_ENGLISH_CHAR_LCD_DISPLAY_SUPPORT
  const Entry punctuation[] = {
      {212122,'!'}, {1112112,'$'}, {122221,'\''}, {121121,'"'},
      {112212,'_'}, {212121,';'}, {222111,':'}, {212212,')'}, {12121,'<'},
      {12111,'&'}, {111212,'>'}
  };
  for (const auto &entry : punctuation) if (pattern == entry.pattern) return entry.character;
#else
  if (pattern == 12121) return '+';
#endif
#ifdef OPTION_NON_ENGLISH_EXTENSIONS
  const Entry extended[] = {
      {12212,197}, {1212,198}, {2222,138}, {22122,209}, {2221,216}, {1122,220},
      {111111,223}, {21211,199}, {11221,208}, {12112,200}, {11211,201}, {221121,142}
  };
  for (const auto &entry : extended) if (pattern == entry.pattern) return entry.character;
#endif
  return -1;
}

static void check(long pattern) {
  const int result = expected(pattern);
  const int before = error_tones;
  assert(convert_cw_number_to_ascii(pattern) == (result < 0 ? '#' : result));
#ifdef OPTION_UNKNOWN_CHARACTER_ERROR_TONE
  assert(error_tones == before + (result < 0 ? 1 : 0));
#else
  assert(error_tones == before);
#endif
}

int main() {
  for (int length = 1; length <= 8; ++length) {
    for (unsigned bits = 0; bits < (1u << length); ++bits) {
      long pattern = 0;
      for (int digit = length - 1; digit >= 0; --digit)
        pattern = pattern * 10 + ((bits >> digit) & 1u) + 1;
      check(pattern);
    }
  }
  check(9);
  check(0);
  check(-1);
  check(123);
  check(LONG_MIN);
  check(LONG_MAX);
  puts("PASS: all 510 dit/dah patterns through eight elements, punctuation/prosign/extended mappings, unknown-character and error-tone behavior");
}
