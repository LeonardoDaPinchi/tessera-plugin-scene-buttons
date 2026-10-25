#include "scene_buttons.h"
#include <algorithm>
#include <cstring>
#include <string>

namespace esphome::scene_buttons {

using tessera::Font;
namespace ui = tessera::ui;

static constexpr int MAX_KEYS = 3;

// The screen's faces for a letter, largest first. A key takes the one its option asks for, or the next smaller one that
// fits (a letter has to fit the key's height and width).
static const Font FACES[] = {Font::VALUE, Font::HEADLINE, Font::TITLE, Font::BODY_LARGE, Font::BODY};
static constexpr int FACE_COUNT = 5;

// Where in FACES the option "text_size" starts looking: "auto" takes the largest that fits.
static int first_face(const char *size) {
  if (!strcmp(size, "small")) return 4;
  if (!strcmp(size, "medium")) return 3;
  if (!strcmp(size, "large")) return 2;
  if (!strcmp(size, "xlarge")) return 1;
  return 0;  // "xxlarge" and "auto"
}

// A kept text (in PSRAM) as a plain one, for a call that takes std::string.
static std::string plain(const tessera::Text &t) { return std::string(t.data(), t.size()); }

// The tile: up to three keys, each a letter, in a row across the tile, or two rows when a row would make them too narrow
// (a 1x1 tile). A tap on a key turns on its scene. create() makes the parts once; a scene has no state of its own to
// show, so nothing changes after that except the colours.
class KeysTile : public tessera::Tile {
 public:
  explicit KeysTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    gap_ = ui::px(8);
    const int pad = ui::px(8);
    accent_ = !strcmp(c.options["key_style"] | "raised", "accent");

    // Only the keys that have a scene.
    static const char *const SCENE_IDS[MAX_KEYS] = {"scene1", "scene2", "scene3"};
    static const char *const LABEL_IDS[MAX_KEYS] = {"label1", "label2", "label3"};
    static const char *const LABEL_DEFAULTS[MAX_KEYS] = {"A", "F", "N"};
    std::string labels[MAX_KEYS];
    int n = 0;
    for (int i = 0; i < MAX_KEYS; i++) {
      const char *scene = c.options[SCENE_IDS[i]] | "";
      if (!scene[0]) continue;
      keys_[n].scene = tessera::Text(scene);
      labels[n] = c.options[LABEL_IDS[i]] | LABEL_DEFAULTS[i];
      n++;
    }
    key_count_ = n;

    if (n == 0) {
      // Nothing chosen yet: say what to do.
      hint_ = ui::label(c.parent, Font::BODY_LARGE, theme::MUTED);
      lv_obj_set_width(hint_, width_ - 2 * pad);
      lv_obj_set_style_text_align(hint_, LV_TEXT_ALIGN_CENTER, 0);
      ui::set_text(hint_, plugin_->text("choose_scenes"));
      lv_obj_set_pos(hint_, pad, (height_ - ui::line_height(Font::BODY_LARGE)) / 2);
      return;
    }

    // One row when each key keeps at least 9 mm of width, else two rows (three keys on a 1x1 tile: two and one).
    const int room_w = width_ - 2 * pad, room_h = height_ - 2 * pad;
    cols_ = n;
    rows_ = 1;
    if (n > 1 && (room_w - (n - 1) * gap_) / n < ui::mm(9)) {
      cols_ = (n + 1) / 2;
      rows_ = 2;
    }
    key_w_ = std::max(1, (room_w - (cols_ - 1) * gap_) / cols_);
    key_h_ = std::max(1, (room_h - (rows_ - 1) * gap_) / rows_);
    const int top = (height_ - (rows_ * key_h_ + (rows_ - 1) * gap_)) / 2;

    // The letter's face: the one asked for, or the next smaller that fits the key's height and its widest letter.
    Font face = Font::BODY;
    for (int f = first_face(c.options["text_size"] | "auto"); f < FACE_COUNT; f++) {
      face = FACES[f];
      int widest = 0;
      for (int i = 0; i < n; i++) widest = std::max(widest, ui::text_width(labels[i], face));
      if (ui::line_height(face) + 2 * ui::px(4) <= key_h_ && widest <= key_w_ - 2 * ui::px(6)) break;
    }

    const theme::Role fill = accent_ ? theme::ACCENT : theme::RAISED;
    const theme::Role ink = accent_ ? theme::ON_ACCENT : theme::INK;
    for (int i = 0; i < n; i++) {
      Key &k = keys_[i];
      k.x = pad + (i % cols_) * (key_w_ + gap_);
      k.y = top + (i / cols_) * (key_h_ + gap_);
      k.block = ui::block(c.parent, fill);
      lv_obj_remove_flag(k.block, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_remove_flag(k.block, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_style_pad_all(k.block, 0, 0);
      lv_obj_set_size(k.block, key_w_, key_h_);
      lv_obj_set_pos(k.block, k.x, k.y);
      k.text = ui::label(k.block, face, ink);
      lv_obj_set_width(k.text, key_w_);
      lv_obj_set_style_text_align(k.text, LV_TEXT_ALIGN_CENTER, 0);
      ui::set_text(k.text, labels[i]);
      lv_obj_set_pos(k.text, 0, (key_h_ - ui::line_height(face)) / 2);
    }
  }

  void on_theme() override {
    if (hint_) ui::set_color(hint_, theme::MUTED);
    for (int i = 0; i < key_count_; i++) {
      ui::set_fill(keys_[i].block, tessera::Tone::NORMAL, accent_ ? theme::ACCENT : theme::RAISED);
      ui::set_color(keys_[i].text, accent_ ? theme::ON_ACCENT : theme::INK);
    }
  }

  void on_tap() override {
    const tessera::Point p = tessera::tap_point();
    if (p.x < 0) return;
    // The key the finger falls on, half of the gap on either side included: a tap between two keys goes to the nearer
    // one. In a single row the keys are most of the tile's height, so a tap above or below one still means it.
    for (int i = 0; i < key_count_; i++) {
      const Key &k = keys_[i];
      if (p.x < k.x - gap_ / 2 || p.x >= k.x + key_w_ + gap_ / 2) continue;
      if (rows_ > 1 && (p.y < k.y - gap_ / 2 || p.y >= k.y + key_h_ + gap_ / 2)) continue;
      tessera::action("scene.turn_on", plain(k.scene));
      return;
    }
  }

 private:
  struct Key {
    lv_obj_t *block = nullptr;
    lv_obj_t *text = nullptr;
    tessera::Text scene;
    int x = 0, y = 0;
  };

  const tessera::Plugin *plugin_;
  int width_ = 0, height_ = 0, gap_ = 0, key_w_ = 0, key_h_ = 0, key_count_ = 0, cols_ = 1, rows_ = 1;
  bool accent_ = false;
  Key keys_[MAX_KEYS];
  lv_obj_t *hint_{};
};

void SceneButtons::setup() {
  // The id is the tile's id in the manifest.
  add_tile("keys", [this]() { return new KeysTile(this); });
}

}  // namespace esphome::scene_buttons
