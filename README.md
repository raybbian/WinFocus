# WinFocus

Focus-follows-mouse for Windows, without auto-raise.

## Behavior

- **Hover** over a window — it receives keyboard focus but stays in its current Z-order
- **Click** a window — it receives focus and is raised to the front
- **Press a key** while hovering — the window is raised to the front

Modifier-only keys (Shift, Ctrl, Alt, Win) do not trigger a raise.

## How it works

WinFocus enables the built-in Windows active window tracking (`SPI_SETACTIVEWINDOWTRACKING`) with auto-raise disabled (`SPI_SETACTIVEWNDTRKZORDER = FALSE`). A low-level keyboard hook listens for keypresses and raises the window under the cursor via `SetWindowPos`. Original system settings are restored on exit.

## Build

Requires Visual Studio 2022 (or any MSVC toolchain). Run `build.bat` from a Developer Command Prompt, or:

```bat
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 -no_logo
cl /nologo /W4 /O2 winfocus.c /link user32.lib /SUBSYSTEM:WINDOWS
```

## Run

```
winfocus.exe
```

Runs as a background process with no window. Stop it with:

```
taskkill /f /im winfocus.exe
```

## Start on login (as admin)

Create a scheduled task from an elevated command prompt:

```
schtasks /create /tn "WinFocus" /tr "C:\path\to\winfocus.exe" /sc onlogon /rl highest /f
```

Remove it with:

```
schtasks /delete /tn "WinFocus" /f
```

## Notes

- A mutex prevents multiple instances from running simultaneously
- Elevated (UAC) windows require WinFocus to also run elevated — use the scheduled task method with `/rl highest`
- Works across multiple monitors
