# Price Clock Wall Case

The FreeCAD macro `price_clock_case.FCMacro` generates:

- `price_clock_front_housing.stl` — 80 mm square front plate and recessed rear cup
- `price_clock_rear_lid.stl` — screw-fastened cover with ESP32 S2 Mini edge guides
- `price_clock_case.FCStd` — editable FreeCAD model

Run it with FreeCAD's bundled command line on macOS:

```sh
"/Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd" \
  "$(pwd)/cad/price_clock_case.FCMacro"
```

## Current dimensions

- Face: 80 × 80 × 3.2 mm, rounded corners
- Wall-box screws: 60 mm horizontal centres, front countersunk clearance holes
- Rear cup: 68 mm outside diameter, 20.5 mm behind the face
- Rear lid: 2 mm thick; total front-to-back depth is about 25.7 mm
- Display aperture: 34 mm; internal ledge sized for a nominal 40 mm PCB
- Panel switch aperture: 12.3 mm; centred 24 mm below the display
- USB-C side slot faces the bottom of the installed faceplate

The Waveshare module and exact switch model have not been mechanically measured.
Before printing the final case, verify that the display PCB is about 40 mm across,
the visible display clears the 34 mm aperture, and the switch requires a 12 mm
panel cutout. The shell provides a display ledge and board guides rather than
assuming undocumented PCB hole positions. The S2 Mini USB connector should point
toward the side slot when installing the rear lid.

Print the front housing with its front face on the build plate. Print the lid
with its flat outer face on the build plate. Use M3 wall-box screws and four
small self-tapping screws suitable for 2.2 mm pilot bores in the internal bosses.