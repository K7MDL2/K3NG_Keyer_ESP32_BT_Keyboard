#ifndef ENCODER_MENU_H
#define ENCODER_MENU_H

#include <cstdint>

struct EncoderMenu {
  static constexpr unsigned MaxControls = 16;
  bool open = false, adjusting = false, touch_down = false;
  uint8_t encoder = 0, selected = 0, count = 0;
  uint8_t controls[MaxControls]{};
  uint32_t activity_ms = 0;
  int drag_x = 0;
  uint8_t selection_pulses = 2;
  int selection_pending = 0;

  void show(uint8_t index, const uint8_t *items, uint8_t size,
            uint8_t active, uint32_t now) {
    encoder = index;
    count = size;
    selected = 0;
    for (uint8_t i = 0; i < size; ++i) {
      controls[i] = items[i];
      if (items[i] == active) selected = i;
    }
    open = true;
    adjusting = touch_down = false;
    activity_ms = now;
    selection_pending = 0;
  }

  bool select(int direction, uint32_t now) {
    if (!count) return false;
    activity_ms = now;
    selection_pending += direction > 0 ? 1 : -1;
    if (selection_pending < selection_pulses && selection_pending > -selection_pulses) return false;
    selection_pending = 0;
    selected = (selected + (direction > 0 ? 1 : count - 1)) % count;
    return true;
  }

  bool expired(uint32_t now) const {
    return open && !touch_down && static_cast<uint32_t>(now - activity_ms) >= 5000;
  }
};

struct EncoderMenuLayout {
  int x, y, width, height, count, selected = 0;
  static constexpr int Header = 22, Gap = 3;
  int center_width() const { return (width - 4 * Gap) / 2; }
  int center_height() const { return height - Header - 2 * Gap; }
  int side_width() const { return (width - center_width() - 4 * Gap) / 2; }
  int side_height() const { return 34; }
  int side(int index) const {
    if (index == selected) return 0;
    if (count > 1 && index == (selected + 1) % count) return 1;
    if (count > 2 && index == (selected + count - 1) % count) return -1;
    return 2;
  }
  bool visible(int index) const { return side(index) != 2; }
  int box_width(int index) const { return index == selected ? center_width() : side_width(); }
  int box_height(int index) const { return index == selected ? center_height() : side_height(); }
  int cell_x(int index) const {
    const int center = x + (width - center_width()) / 2;
    return side(index) < 0 ? center - Gap - side_width() :
        side(index) > 0 ? center + center_width() + Gap : center;
  }
  int cell_y(int index) const {
    return y + Header + Gap + (center_height() - box_height(index)) / 2;
  }
  bool close_hit(int px, int py) const {
    return px >= x + width - 60 && px < x + width &&
           py >= y && py < y + Header;
  }
  int hit(int px, int py) const {
    for (int i = 0; i < count; ++i)
      if (visible(i) && px >= cell_x(i) && px < cell_x(i) + box_width(i) &&
          py >= cell_y(i) && py < cell_y(i) + box_height(i)) return i;
    return -1;
  }
};

#endif
