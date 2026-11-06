#include "keyer_morse.h"

int keyer_morse_character(long pattern, const MorseLookupOptions &options) {
  switch (pattern) {
    case 12: return 'A';
    case 2111: return 'B';
    case 2121: return 'C';
    case 211: return 'D';
    case 1: return 'E';
    case 1121: return 'F';
    case 221: return 'G';
    case 1111: return 'H';
    case 11: return 'I';
    case 1222: return 'J';
    case 212: return 'K';
    case 1211: return 'L';
    case 22: return 'M';
    case 21: return 'N';
    case 222: return 'O';
    case 1221: return 'P';
    case 2212: return 'Q';
    case 121: return 'R';
    case 111: return 'S';
    case 2: return 'T';
    case 112: return 'U';
    case 1112: return 'V';
    case 122: return 'W';
    case 2112: return 'X';
    case 2122: return 'Y';
    case 2211: return 'Z';
    case 22222: return '0';
    case 12222: return '1';
    case 11222: return '2';
    case 11122: return '3';
    case 11112: return '4';
    case 11111: return '5';
    case 21111: return '6';
    case 22111: return '7';
    case 22211: return '8';
    case 22221: return '9';
    case 112211: return '?';
    case 21121: return '/';
    case 221122: return ',';
    case 121212: return '.';
    case 122121: return '@';
    case 222222: return '\\';
    case 21112: return '=';
    case 211112: return '-';
    case 9: return ' ';
    case 12121: return options.keyboard_lcd ? '<' : '+';
    case 2111212: return options.prosign_start ? options.prosign_start + 3 : '*';
    case 21221: return options.prosign_start ? options.prosign_start + 6 : '(';
  }

  if (options.prosign_start) {
    switch (pattern) {
      case 1212: if (!options.non_english) return options.prosign_start + 1; break;
      case 12111: return options.prosign_start + 2;
      case 21211211: return options.prosign_start + 4;
      case 21212: return options.prosign_start + 5;
      case 211222: return options.prosign_start + 7;
      case 111212: return options.prosign_start + 8;
      case 11121: return options.prosign_start + 9;
      case 11111111: return options.prosign_start + 10;
    }
  }

  if (options.keyboard_lcd) {
    switch (pattern) {
      case 212122: return '!';
      case 1112112: return '$';
      case 12111: return '&';
      case 122221: return '\'';
      case 121121: return '"';
      case 112212: return '_';
      case 212121: return ';';
      case 222111: return ':';
      case 212212: return ')';
      case 111212: return '>';
    }
  }

  if (options.non_english) {
    switch (pattern) {
      case 12212: return 197;
      case 1212: return 198;
      case 2222: return 138;
      case 22122: return 209;
      case 2221: return 216;
      case 1122: return 220;
      case 111111: return 223;
      case 21211: return 199;
      case 11221: return 208;
      case 12112: return 200;
      case 11211: return 201;
      case 221121: return 142;
    }
  }
  return -1;
}
