# Furniture

A physics-based bounce sequencer. Balls bounce around a canvas, triggering MIDI notes when they hit zones. Includes an arpeggiator with 8 play modes, sync/free rate, pendulum, and ratchet.

**[Try the web version](https://gregoryhbowler.github.io/furniture/)**

## Web Version

Open `index.html` in a browser (Chrome/Edge recommended for Web MIDI support).

- **Volume slider** — set to 0 to use as a pure MIDI sequencer
- **MIDI Output** — send notes to hardware, Ableton, or other DAWs via virtual MIDI (e.g. IAC Driver on Mac, loopMIDI on Windows)
- **Arpeggiator** — 8 play modes, BPM sync with divisions (1/64 to 1 bar + triplets), free ms rate, pendulum, ratchet

## VST Plugin (macOS)

### Building from source

Requires CMake 3.22+ and a C++17 compiler (Xcode).

```bash
cd plugin/build
cmake ..
cmake --build .
```

The built plugin is automatically installed to `~/Library/Audio/Plug-Ins/VST3/` and `~/Library/Audio/Plug-Ins/Components/` (AU).

### Installing a pre-built release

1. Download the latest `.zip` from [Releases](https://github.com/gregoryhbowler/furniture/releases)
2. Unzip and copy `Furniture.vst3` to `~/Library/Audio/Plug-Ins/VST3/`
3. For AU: copy `Furniture.component` to `~/Library/Audio/Plug-Ins/Components/`

#### Unsigned plugin — macOS Gatekeeper

Since this is not notarized by Apple, macOS will block it. To allow it:

**Method 1 — System Settings (recommended):**
1. Try to load the plugin in your DAW — it will fail with a security warning
2. Go to **System Settings → Privacy & Security**
3. Scroll down — you'll see a message about "Furniture" being blocked
4. Click **Allow Anyway**
5. Reload the plugin in your DAW

**Method 2 — Terminal:**
```bash
# Remove the quarantine attribute
xattr -d com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Furniture.vst3
# For AU:
xattr -d com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Furniture.component
```

**Method 3 — Ad-hoc re-sign (if "damaged" error):**
```bash
codesign --force --deep --sign - ~/Library/Audio/Plug-Ins/VST3/Furniture.vst3
```

### Windows

Not yet tested. The CMakeLists.txt should work with MSVC — contributions welcome.

## Arpeggiator

The arp cycles through all zones on the canvas. Always latched.

| Setting | Options |
|---------|---------|
| Sync | BPM-synced divisions or free ms rate |
| Divisions | 1/64, 1/32, 1/16, 1/8, 1/4, 1/2, 1 bar + triplets |
| Free rate | 10ms – 4000ms |
| Play modes | First Placed, Up, Down, Random, Left→Right, Right→Left, Vertical Up, Vertical Down |
| Pendulum | Bounce back and forth instead of looping |
| Ratchet | 0–16 extra repeats per step |

In the VST, BPM is read from the host transport. In the web version, set BPM manually.
