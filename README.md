# Scene buttons

A light tile with up to three small keys on it. Tap a key to turn on a scene; tap the "..." key for a card with a brightness slider and a power key; tap the rest of
the tile to switch the light: on turns it off, off turns it on (or presses buttons of your choice). It is the Tessera version of a Bubble Card button
with sub-buttons.

## Set up

1. Add the plugin to a screen in Tessera. The screen builds once.
2. In Layout, place the **Scene buttons** tile from the library's Plugins group, 2 columns wide or more, and choose
   the light in the inspector.
3. Optional: under **Tap when off**, write a button a tap presses to turn the light on, such as
   `input_button.living_room_on`; under **Tap when on**, a button that turns it off. Leave them empty and a tap turns
   the light on or off itself.
4. For each key, write a scene entity (`scene.living_room_living_room_normal_warm`) and the letter on the key. A key
   with no scene is not shown.

## The card

The "..." key on the tile opens a card of this plugin's own, since a plugin tile cannot tell a long press from a tap: the light's brightness as a slider with its percentage
over it, and a key that turns the light on or off. Drag the slider and let go to set the brightness; all the way down
turns the light off. It is not Tessera's own light card (a plugin cannot open that one), so it has no colour or
white-shade rows. It needs a Tessera with plugin API 0.12 or later.

## Good to know

- The tile shows the light's name, and whether it is on and how bright.
- A tap on a key turns on its scene; it does not change the tile's colour, because a scene has no state of its own that
  says it is still the one in use.
- Keys are at least 7 mm across so a finger finds them. On a small tile the name gets less room.
- It calls these Home Assistant actions: `input_button.press`, `light.turn_on`, `light.turn_off`, `light.toggle` and `scene.turn_on`.
