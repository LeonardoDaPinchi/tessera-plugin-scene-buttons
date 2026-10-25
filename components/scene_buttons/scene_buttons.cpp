#include "scene_buttons.h"
#include <algorithm>
#include <string>

namespace esphome::scene_buttons {

using tessera::Font;
namespace ui = tessera::ui;

static constexpr int MAX_KEYS = 3;

// A kept text (in PSRAM) as a plain one, for a call that takes std::string.
static std::string plain(const tessera::Text &t) { return std::string(t.data(), t.size()); }

// The tile: the light's name and state on the left, up to three keys on the right. A tap on a key turns on its scene;
// a tap anywhere else presses the chosen input_button, or toggles the light when none is chosen.
class ButtonsTile : public tessera::Tile {
 public:
  explicit ButtonsTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    pad_ = ui::px(8);
    gap_ = ui::px(8);
    entity_ = tessera::Text(c.entity);
    name_text_ = tessera::Text(c.name);
    press_ = tessera::Text(c.options["press"] | "");

    // The keys: only those with a scene, each a square a finger finds, at the right edge, in the middle of the height.
    static const char *const SCENE_IDS[MAX_KEYS] = {"scene1", "scene2", "scene3"};
    static const char *const LABEL_IDS[MAX_KEYS] = {"label1", "label2", "label3"};
    static const char *const LABEL_DEFAULTS[MAX_KEYS] = {"N", "A", "F"};
    key_size_ = std::min(ui::mm(12), height_ - 2 * pad_);
    if (key_size_ < ui::mm(7)) key_size_ = std::min(ui::mm(7), height_);
    int n = 0;
    for (int i = 0; i < MAX_KEYS; i++) {
      const char *scene = c.options[SCENE_IDS[i]] | "";
      if (!scene[0]) continue;
      Key &k = keys_[n++];
      k.scene = tessera::Text(scene);
      const char *label = c.options[LABEL_IDS[i]] | LABEL_DEFAULTS[i];
      k.block = ui::block(c.parent, theme::RAISED);
      lv_obj_remove_flag(k.block, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_remove_flag(k.block, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_style_pad_all(k.block, 0, 0);
      lv_obj_set_size(k.block, key_size_, key_size_);
      k.text = ui::label(k.block, Font::BODY_LARGE, theme::INK);
      lv_obj_set_width(k.text, key_size_);
      lv_obj_set_style_text_align(k.text, LV_TEXT_ALIGN_CENTER, 0);
      ui::set_text(k.text, label);
      lv_obj_set_pos(k.text, 0, (key_size_ - ui::line_height(Font::BODY_LARGE)) / 2);
    }
    key_count_ = n;
    // Right-aligned: the last key touches the padding, the first is furthest left.
    keys_left_ = width_ - pad_;
    for (int i = n - 1; i >= 0; i--) {
      keys_left_ -= key_size_;
      keys_[i].x = keys_left_;
      lv_obj_set_pos(keys_[i].block, keys_[i].x, (height_ - key_size_) / 2);
      keys_left_ -= gap_;
    }
    // What is left of the width is the words' room.
    const int room = std::max(ui::mm(10), (n ? keys_left_ + gap_ : width_) - pad_);
    name_ = ui::label(c.parent, Font::BODY_LARGE, theme::INK);
    lv_obj_set_width(name_, room - (n ? gap_ : 0));
    state_line_ = ui::label(c.parent, Font::BODY, theme::MUTED);
    lv_obj_set_width(state_line_, room - (n ? gap_ : 0));
    const int block = ui::line_height(Font::BODY_LARGE) + ui::line_height(Font::BODY);
    const int top = std::max(0, (height_ - block) / 2);
    lv_obj_set_pos(name_, pad_, top);
    lv_obj_set_pos(state_line_, pad_, top + ui::line_height(Font::BODY_LARGE));
    paint();
  }

  void on_state(JsonObjectConst data) override {
    if (data["state"].isNull()) return;
    state_ = tessera::Text(data["state"] | "");
    if (!data["name"].isNull() && name_text_.size() == 0) name_text_ = tessera::Text(data["name"] | "");
    // Home Assistant's brightness is 0..255; the tile says a percentage.
    const int b = data["attributes"]["brightness"] | -1;
    brightness_pct_ = b < 0 ? -1 : (b * 100 + 127) / 255;
    paint();
  }

  void on_theme() override {
    ui::set_color(name_, theme::INK);
    ui::set_color(state_line_, theme::MUTED);
    for (int i = 0; i < key_count_; i++) {
      ui::set_fill(keys_[i].block, tessera::Tone::NORMAL, theme::RAISED);
      ui::set_color(keys_[i].text, theme::INK);
    }
  }

  void on_tap() override {
    const tessera::Point p = tessera::tap_point();
    // A key: the finger's x falls on it, its gap included (the keys stand in the middle of the height; a tap above or
    // below one on a low tile still means that key, as the key is most of the height).
    if (p.x >= 0) {
      for (int i = 0; i < key_count_; i++) {
        const Key &k = keys_[i];
        if (p.x >= k.x - gap_ / 2 && p.x <= k.x + key_size_ + gap_ / 2) {
          tessera::action("scene.turn_on", plain(k.scene));
          return;
        }
      }
    }
    // Anywhere else: the button chosen for the tile, or the light itself.
    if (press_.size() > 0) {
      tessera::action("input_button.press", plain(press_));
    } else if (entity_.size() > 0) {
      tessera::action("light.toggle", plain(entity_));
    }
  }

 private:
  struct Key {
    lv_obj_t *block = nullptr;
    lv_obj_t *text = nullptr;
    tessera::Text scene;
    int x = 0;
  };

  void paint() {
    ui::set_text(name_, plain(name_text_));
    std::string line;
    const std::string s = plain(state_);
    if (entity_.size() == 0) {
      line = plugin_->text("choose_light");
    } else if (s == "on") {
      line = brightness_pct_ >= 0 ? tessera::fill(plugin_->text("on_pct"), "pct", std::to_string(brightness_pct_))
                                  : plugin_->text("on");
      if (!ui::covers(line, Font::BODY)) line = plugin_->text("on");
    } else if (s == "off") {
      line = plugin_->text("off");
    } else if (s == "unavailable" || s == "unknown") {
      line = plugin_->text("unavailable");
    }
    ui::set_text(state_line_, line);
    ui::set_tone(state_line_, s == "on" ? tessera::Tone::BUSY : tessera::Tone::NORMAL, theme::MUTED);
  }

  const tessera::Plugin *plugin_;
  int width_ = 0, height_ = 0, pad_ = 0, gap_ = 0, key_size_ = 0, key_count_ = 0, keys_left_ = 0;
  int brightness_pct_ = -1;
  tessera::Text entity_, name_text_, press_, state_;
  Key keys_[MAX_KEYS];
  lv_obj_t *name_{}, *state_line_{};
};

void SceneButtons::setup() {
  // The id is the tile's id in the manifest.
  add_tile("buttons", [this]() { return new ButtonsTile(this); });
}

}  // namespace esphome::scene_buttons
