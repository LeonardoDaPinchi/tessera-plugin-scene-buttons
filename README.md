# Scene buttons

A tile of up to three keys side by side, each a letter. Tap a key to turn on a scene. It is the row of sub-buttons of
a Bubble Card button: put Tessera's own light tile beside it for the light itself (a tap switches it, a hold opens its
card).

## Set up

1. Add the plugin to a screen in Tessera. The screen builds once.
2. In Layout, place the **Scene buttons** tile from the library's Plugins group and give it the size you want, from 1x1 to
   4x2. 2 columns by 1 row puts the three keys in one row; on a 1x1 tile they stand in two rows.
3. In the inspector, write a scene entity for each key (`scene.living_room_living_room_normal_warm`) and the letter on
   it. A key with no scene is not shown, and the others share the width.
4. Optional: choose the **Size of the letters** (Auto takes the largest that fits the key) and the **Key colour**
   (a raised grey field, or the screen's accent colour). The tile's own background colour is set in the editor, as for
   every tile.

## Good to know

- The keys stand left to right in the order of their numbers; the letters default to A, F and N.
- A scene has no state of its own that says it is still the one in use, so a key does not change colour when pressed.
- On a very small tile the keys get small; a larger tile (2x1 or more) keeps them easy to hit.
- It calls one Home Assistant action: `scene.turn_on`.
