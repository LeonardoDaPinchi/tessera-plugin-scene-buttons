# Scene buttons

A light tile with up to three small keys on it. Tap a key to turn on a scene; tap the rest of the tile to press a
button of your choice (an `input_button`), or to toggle the light. It is the Tessera version of a Bubble Card button
with sub-buttons.

## Set up

1. Add the plugin to a screen in Tessera. The screen builds once.
2. In Layout, place the **Scene buttons** tile from the library's Plugins group, 2 columns wide or more, and choose
   the light in the inspector.
3. Under **Tap on the tile**, write the entity a tap presses, such as `input_button.living_room_on`. Leave it empty and a
   tap toggles the light.
4. For each key, write a scene entity (`scene.living_room_living_room_normal_warm`) and the letter on the key. A key
   with no scene is not shown.

## Good to know

- The tile shows the light's name, and whether it is on and how bright.
- A tap on a key turns on its scene; it does not change the tile's colour, because a scene has no state of its own that
  says it is still the one in use.
- Keys are at least 7 mm across so a finger finds them. On a small tile the name gets less room.
- It calls three Home Assistant actions: `input_button.press`, `light.toggle` and `scene.turn_on`.
