# Led'o'clock Enclosure

This folder contains the 3D printable enclosure for the Led'o'clock project. The case is designed to house the ESP32-C3 module and a 12-LED WS2812B ring, with a clean **64 × 64 mm** square footprint.

## Files

| File | Description                                                                                                 |
| :--- |:------------------------------------------------------------------------------------------------------------|
| `ledoclock.FCStd` | FreeCAD source file — open this to modify the design.                                                       |
| `ledoclock.3mf` | Bambu Studio project file — ready to slice, includes print settings.                                        |
| `ledoclock-box bottom.stl` | Main body: houses the PCB and the ESP32-C3.                                                                 |
| `ledoclock-lid.stl` | Top lid: snaps onto the body, houses the LED ring and acts as a light diffuser frame.                       |
| `ledoclock-ring retainer.stl` | Thin ring retainer: keeps the LED ring flush inside the lid cavity, to avoid it to fall into the main body. |

## Dimensions

| Part | W × D × H |
| :--- | :--- |
| Box bottom | 64 × 64 × 25 mm |
| Lid | 64 × 64 × 4 mm |
| Ring retainer | 64 × 64 × 0.8 mm |
| **Assembled** | **64 × 64 × 29 mm** |

## Recommended Print Settings

The `.3mf` file embeds all settings below and was validated on a **Bambu Lab P1S**.

| Parameter | Value |
| :--- | :--- |
| Infill | 15 % — Gyroid |
| Supports | **Lid only** — Tree (Auto) |


> Any standard FDM printer with a **≥ 180 × 180 mm** bed can print all three parts. Non-Bambu users should reproduce the settings above in their own slicer.
