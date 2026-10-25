# Scene buttons

A tile of up to three keys side by side, each a letter. Tap a key to turn on a scene. It is the row of sub-buttons of
a Bubble Card button: put Tessera's own light tile beside it for the light itself (a tap switches it, a hold opens its
card).

## Set up

1. Add the plugin to a screen in Tessera. The screen builds once.
2. In Layout, place the **Scene buttons** tile from the library's Plugins group. It is 2 columns by 1 row.
3. In the inspector, write a scene entity for each key (`scene.living_room_living_room_normal_warm`) and the letter on
   it. A key with no scene is not shown, and the others share the width.

## Good to know

- The keys stand left to right in the order of their numbers; the letters default to A, F and N.
- A scene has no state of its own that says it is still the one in use, so a key does not change colour when pressed.
- Keys are at least 7 mm high so a finger finds them.
- It calls one Home Assistant action: `scene.turn_on`.
