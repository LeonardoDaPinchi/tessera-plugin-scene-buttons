#include "scene_buttons.h"
#include <algorithm>
#include <string>

namespace esphome::scene_buttons {

using tessera::Font;
namespace ui = tessera::ui;

static constexpr int MAX_KEYS = 3;

// A kept text (in PSRAM) as a plain one, for a call that takes std::string.
static std::string plain(const tessera::Text &t) { return std::string(t.data(), t.size()); }

// The tile: up to three keys side by side across its whole width, each a letter. A tap on a key turns on its scene.
// create() makes the parts once; a scene has no state of its own to show, so nothing changes after that except colours.
class KeysTile : public tessera::Tile {
 public:
  explicit KeysTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    gap_ = ui::px(8);
    const int pad = ui::px(8);

    // Only the keys that have a scene.
    static const char *const SCENE_IDS[MAX_KEYS] = {"scene1", "scene2", "scene3"};
    static const char *const LABEL_IDS[MAX_KEYS] = {"label1", "label2", "label3"};
    static const char *const LABEL_DEFAULTS[MAX_KEYS] = {"A", "F", "N"};
    const char *labels[MAX_KEYS] = {};
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

    // The keys share the width and stand as high as the tile allows, so a finger finds them.
    key_w_ = (width_ - 2 * pad - (n - 1) * gap_) / n;
    key_h_ = std::max(ui::mm(7), height_ - 2 * pad);
    key_h_ = std::min(key_h_, height_);
    // The letter in the largest of the screen's faces the key is high enough for.
    Font face = Font::BODY_LARGE;
    for (Font f : {Font::VALUE, Font::HEADLINE, Font::TITLE}) {
      if (ui::line_height(f) + 2 * ui::px(6) <= key_h_) {
        face = f;
        break;
      }
    }
    for (int i = 0; i < n; i++) {
      Key &k = keys_[i];
      k.x = pad + i * (key_w_ + gap_);
      k.block = ui::block(c.parent, theme::RAISED);
      lv_obj_remove_flag(k.block, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_remove_flag(k.block, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_style_pad_all(k.block, 0, 0);
      lv_obj_set_size(k.block, key_w_, key_h_);
      lv_obj_set_pos(k.block, k.x, (height_ - key_h_) / 2);
      k.text = ui::label(k.block, face, theme::INK);
      lv_obj_set_width(k.text, key_w_);
      lv_obj_set_style_text_align(k.text, LV_TEXT_ALIGN_CENTER, 0);
      ui::set_text(k.text, labels[i]);
      lv_obj_set_pos(k.text, 0, (key_h_ - ui::line_height(face)) / 2);
    }
  }

  void on_theme() override {
    if (hint_) ui::set_color(hint_, theme::MUTED);
    for (int i = 0; i < key_count_; i++) {
      ui::set_fill(keys_[i].block, tessera::Tone::NORMAL, theme::RAISED);
      ui::set_color(keys_[i].text, theme::INK);
    }
  }

  void on_tap() override {
    const tessera::Point p = tessera::tap_point();
    if (p.x < 0) return;
    // The key the finger's x falls on, its half of the gap on either side included: a tap between two keys goes to the
    // nearer one, and a tap above or below a key still means it, since the keys are most of the tile's height.
    for (int i = 0; i < key_count_; i++) {
      const Key &k = keys_[i];
      if (p.x >= k.x - gap_ / 2 && p.x < k.x + key_w_ + gap_ / 2) {
        tessera::action("scene.turn_on", plain(k.scene));
        return;
      }
    }
  }

 private:
  struct Key {
    lv_obj_t *block = nullptr;
    lv_obj_t *text = nullptr;
    tessera::Text scene;
    int x = 0;
  };

  const tessera::Plugin *plugin_;
  int width_ = 0, height_ = 0, gap_ = 0, key_w_ = 0, key_h_ = 0, key_count_ = 0;
  Key keys_[MAX_KEYS];
  lv_obj_t *hint_{};
};

void SceneButtons::setup() {
  // The id is the tile's id in the manifest.
  add_tile("keys", [this]() { return new KeysTile(this); });
}

}  // namespace esphome::scene_buttons
