#include "scene_buttons.h"
#include <algorithm>
#include <string>

namespace esphome::scene_buttons {

using tessera::Font;
namespace ui = tessera::ui;

static constexpr int MAX_SCENES = 3;
// Material Design Icons "dots-horizontal", in Tessera's icon set.
static const uint32_t ICON_MORE = 0xF01D8;

// A kept text (in PSRAM) as a plain one, for a call that takes std::string.
static std::string plain(const tessera::Text &t) { return std::string(t.data(), t.size()); }

// The card a tap on the "..." key opens: the light's brightness as a slider with its percentage over it, and a key that
// turns the light on or off. It is the plugin's own, drawn with the screen's fonts and colours; it is not Tessera's
// light card, which a plugin cannot open.
class LightCard : public tessera::Card {
 public:
  explicit LightCard(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void open(const tessera::CardContext &c) override {
    width_ = c.width;
    height_ = c.height;
    entity_ = tessera::Text(c.entity);
    const int pad = ui::px(16), gap = ui::px(14);
    const int room = std::max(ui::mm(20), width_ - 2 * pad);
    slider_h_ = ui::mm(11);
    key_h_ = std::max(ui::mm(12), ui::line_height(Font::BODY_LARGE) + ui::px(16));

    // The percentage in the largest face the card is high enough for.
    Font face = Font::VALUE;
    int total = 0;
    for (Font f : {Font::VALUE, Font::HEADLINE, Font::TITLE}) {
      face = f;
      total = ui::line_height(f) + gap + slider_h_ + gap + key_h_;
      if (total <= height_) break;
    }
    const int top = std::max(0, (height_ - total) / 2);

    value_ = ui::label(c.parent, face, theme::INK);
    lv_obj_set_width(value_, width_);
    lv_obj_set_style_text_align(value_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(value_, 0, top);

    // The slider is an LVGL slider of the card's own: it keeps its drags. The value goes to Home Assistant when the
    // finger lets go, not on every step of the drag.
    slider_ = lv_slider_create(c.parent);
    lv_slider_set_range(slider_, 0, 100);
    lv_obj_set_size(slider_, room, slider_h_);
    lv_obj_set_pos(slider_, (width_ - room) / 2, top + ui::line_height(face) + gap);
    lv_obj_set_style_radius(slider_, slider_h_ / 2, LV_PART_MAIN);
    lv_obj_set_style_radius(slider_, slider_h_ / 2, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider_, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider_, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider_, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_add_event_cb(slider_, &LightCard::slider_event, LV_EVENT_ALL, this);

    // The key: as wide as the slider, a finger high at least.
    key_x_ = (width_ - room) / 2;
    key_y_ = top + ui::line_height(face) + gap + slider_h_ + gap;
    key_w_ = room;
    key_ = ui::block(c.parent, theme::ACCENT);
    lv_obj_remove_flag(key_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(key_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(key_, 0, 0);
    lv_obj_set_style_radius(key_, key_h_ / 2, 0);
    lv_obj_set_size(key_, key_w_, key_h_);
    lv_obj_set_pos(key_, key_x_, key_y_);
    key_text_ = ui::label(key_, Font::BODY_LARGE, theme::ON_ACCENT);
    lv_obj_set_width(key_text_, key_w_);
    lv_obj_set_style_text_align(key_text_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(key_text_, 0, (key_h_ - ui::line_height(Font::BODY_LARGE)) / 2);
    colour_slider();
    paint();
  }

  void on_state(JsonObjectConst data) override {
    if (data["state"].isNull()) return;
    state_ = tessera::Text(data["state"] | "");
    const int b = data["attributes"]["brightness"] | -1;
    // Home Assistant's brightness is 0..255; the slider says a percentage. A finger on the slider keeps its own value.
    if (!dragging_) pct_ = b < 0 ? -1 : (b * 100 + 127) / 255;
    paint();
  }

  void on_theme() override {
    ui::set_color(value_, theme::INK);
    ui::set_color(key_text_, theme::ON_ACCENT);
    ui::set_fill(key_, tessera::Tone::NORMAL, theme::ACCENT);
    colour_slider();
  }

  // A tap on the key switches the light; a tap anywhere else does nothing.
  void on_tap() override {
    const tessera::Point p = tessera::tap_point();
    const int slack = ui::px(8);
    if (entity_.size() == 0 || p.x < key_x_ - slack || p.x > key_x_ + key_w_ + slack) return;
    if (p.y < key_y_ - slack || p.y > key_y_ + key_h_ + slack) return;
    tessera::action(plain(state_) == "on" ? "light.turn_off" : "light.turn_on", plain(entity_));
  }

 private:
  // The slider's finger came down, moved or went up.
  static void slider_event(lv_event_t *e) {
    auto *self = static_cast<LightCard *>(lv_event_get_user_data(e));
    lv_obj_t *slider = lv_event_get_target_obj(e);
    const lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
      self->dragging_ = true;
      self->pct_ = lv_slider_get_value(slider);
      self->paint();
    } else if (code == LV_EVENT_RELEASED && self->dragging_) {
      self->dragging_ = false;
      self->send(lv_slider_get_value(slider));
    }
  }

  // Zero turns the light off; anything else sets its brightness (which turns it on).
  void send(int pct) {
    if (entity_.size() == 0) return;
    pct_ = pct;
    if (pct <= 0) {
      state_ = tessera::Text("off");
      tessera::action("light.turn_off", plain(entity_));
    } else {
      state_ = tessera::Text("on");
      tessera::action("light.turn_on", plain(entity_), "brightness_pct", std::to_string(pct));
    }
    paint();
  }

  void colour_slider() {
    lv_obj_set_style_bg_color(slider_, ui::color(theme::TRACK), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_, ui::color(theme::ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_, ui::color(theme::ACCENT), LV_PART_KNOB);
  }

  void paint() {
    const std::string s = plain(state_);
    const bool on = s == "on";
    // An "on" light that says no brightness (a plain switch of a lamp) has nothing for a slider to show.
    const bool dimmable = !(on && pct_ < 0);
    std::string line;
    if (s == "unavailable" || s == "unknown") line = plugin_->text("unavailable");
    else if (on && pct_ < 0) line = plugin_->text("on");
    else if (on || dragging_) line = tessera::fill(plugin_->text("pct"), "pct", std::to_string(std::max(pct_, 0)));
    else line = plugin_->text("off");
    ui::set_text(value_, line);
    if (dimmable) lv_obj_remove_flag(slider_, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(slider_, LV_OBJ_FLAG_HIDDEN);
    if (!dragging_) lv_slider_set_value(slider_, on ? std::max(pct_, 0) : 0, LV_ANIM_OFF);
    ui::set_text(key_text_, plugin_->text(on ? "power_off" : "power_on"));
  }

  const tessera::Plugin *plugin_;
  int width_ = 0, height_ = 0, slider_h_ = 0, key_h_ = 0, key_x_ = 0, key_y_ = 0, key_w_ = 0;
  int pct_ = -1;
  bool dragging_ = false;
  tessera::Text entity_, state_;
  lv_obj_t *value_{}, *slider_{}, *key_{}, *key_text_{};
};

// The tile: the light's name and state on the left, up to three scene keys and a "..." key on the right. A tap on a scene
// key turns on its scene; the "..." key opens the light's card; a tap anywhere else switches the light.
class ButtonsTile : public tessera::Tile {
 public:
  explicit ButtonsTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    tile_ = c.tile;
    pad_ = ui::px(8);
    gap_ = ui::px(8);
    entity_ = tessera::Text(c.entity);
    name_text_ = tessera::Text(c.name);
    press_ = tessera::Text(c.options["press"] | "");
    press_off_ = tessera::Text(c.options["press_off"] | "");

    // The keys: only the scenes that are set, each a square a finger finds, at the right edge, in the middle of the
    // height, then the "..." key last. The "..." key is there only once a light is chosen.
    static const char *const SCENE_IDS[MAX_SCENES] = {"scene1", "scene2", "scene3"};
    static const char *const LABEL_IDS[MAX_SCENES] = {"label1", "label2", "label3"};
    static const char *const LABEL_DEFAULTS[MAX_SCENES] = {"N", "A", "F"};
    key_size_ = std::min(ui::mm(12), height_ - 2 * pad_);
    if (key_size_ < ui::mm(7)) key_size_ = std::min(ui::mm(7), height_);
    int n = 0;
    for (int i = 0; i < MAX_SCENES; i++) {
      const char *scene = c.options[SCENE_IDS[i]] | "";
      if (!scene[0]) continue;
      make_key(c.parent, n++, scene, c.options[LABEL_IDS[i]] | LABEL_DEFAULTS[i], false);
    }
    if (entity_.size() > 0) make_key(c.parent, n++, "", "", true);
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
          if (k.more) tessera::open_card(plugin_->plugin_id(), "light", plain(entity_), tile_);
          else tessera::action("scene.turn_on", plain(k.scene));
          return;
        }
      }
    }
    // Anywhere else the tap switches the light. On: the "when on" button, or turn it off. Off: the "when off" button, or
    // turn it on. A state that is neither (unavailable, unknown) toggles, as Home Assistant's own tile would.
    const std::string st = plain(state_);
    if (st == "on") {
      if (press_off_.size() > 0) tessera::action("input_button.press", plain(press_off_));
      else if (entity_.size() > 0) tessera::action("light.turn_off", plain(entity_));
    } else if (press_.size() > 0) {
      tessera::action("input_button.press", plain(press_));
    } else if (entity_.size() > 0) {
      tessera::action(st == "off" ? "light.turn_on" : "light.toggle", plain(entity_));
    }
  }

 private:
  struct Key {
    lv_obj_t *block = nullptr;
    lv_obj_t *text = nullptr;
    tessera::Text scene;
    int x = 0;
    bool more = false;
  };

  // One key: a raised square with a letter on it, or the "..." icon for the key that opens the card.
  void make_key(lv_obj_t *parent, int n, const char *scene, const char *label, bool more) {
    Key &k = keys_[n];
    k.scene = tessera::Text(scene);
    k.more = more;
    k.block = ui::block(parent, theme::RAISED);
    lv_obj_remove_flag(k.block, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(k.block, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(k.block, 0, 0);
    lv_obj_set_size(k.block, key_size_, key_size_);
    const Font face = more ? Font::ICON_SMALL : Font::BODY_LARGE;
    k.text = ui::label(k.block, face, theme::INK);
    lv_obj_set_width(k.text, key_size_);
    lv_obj_set_style_text_align(k.text, LV_TEXT_ALIGN_CENTER, 0);
    ui::set_text(k.text, more ? ui::icon(ICON_MORE) : std::string(label));
    lv_obj_set_pos(k.text, 0, (key_size_ - ui::line_height(face)) / 2);
  }

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
  int width_ = 0, height_ = 0, pad_ = 0, gap_ = 0, key_size_ = 0, key_count_ = 0, keys_left_ = 0, tile_ = -1;
  int brightness_pct_ = -1;
  tessera::Text entity_, name_text_, press_, press_off_, state_;
  Key keys_[MAX_SCENES + 1];
  lv_obj_t *name_{}, *state_line_{};
};

void SceneButtons::setup() {
  // The ids are the manifest's.
  add_tile("buttons", [this]() { return new ButtonsTile(this); });
  add_card("light", [this]() { return new LightCard(this); });
}

}  // namespace esphome::scene_buttons
