# Changelog

## 0.5.0
- The tile can be resized from 1x1 to 4x2 (before it was 2x1 only, which the editor did not let you choose). On a
  1x1 tile the keys stand in two rows.
- New options: **Size of the letters** (auto, small up to largest) and **Key colour** (raised or accent). Nothing to
  change: the defaults look as before.

## 0.4.0
- The tile is now only the keys: up to three letters on a 2x1 tile, each turning on a scene. The light, the "..." key,
  the card and the tap on the tile are gone: use Tessera's own light tile beside it.
- Place the tile again and fill in the scenes: the old tile (`buttons`) is replaced by a new one (`keys`), and its
  light, tap and brightness options no longer exist. The letters now default to A, F and N.

## 0.3.0
- A new "..." key on the tile opens a card with a brightness slider and a power key for the light.
- Needs a Tessera with plugin API 0.12 or later; an older one stops the build with a message.

## 0.2.0
- A tap on the tile now switches the light: on turns it off, off turns it on. Before, it only pressed one button.
- New option **Tap when on**. The option for the tile's tap is now called **Tap when off**. If you filled it in, nothing
  changes for turning on; leave **Tap when on** empty and a tap turns the light off.

## 0.1.0
- First version: a light tile with up to three keys that turn on scenes, and a tap that presses an input_button or
  toggles the light.
