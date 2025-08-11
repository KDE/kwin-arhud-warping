# KWin Effect to warp certain outputs for ArHUD

There are two 'effects' here:
- ClassicArHudEffect: this is the effect that warps HUD alongside the ArHUD.
- MiniARHUDEffect: currently, this is only the moved implementation from oxygen-ui and it handles the warping of hud in all other cases that ClassicArHudEffect doesn't.

# Build

```
mkdir build
cd build
cd build && cmake .. -DCMAKE_INSTALL_PREFIX=~/kwin-dev-scripts/usr -GNinja
ninja install
```
