

<p  align="center">

<img align=center src="https://raw.githubusercontent.com/itheus/termite/refs/heads/main/assets/icon.png"  width="120"  alt="Termite icon">

</p>

  

<h1  align="center">Termite</h1>

  

<p  align="center"><strong>The tiny terminal that lives under your KDE desktop.</strong></p>

  

<p  align="center">

<img  src="https://raw.githubusercontent.com/itheus/termite/refs/heads/main/demo.gif"  width="720"  alt="Termite demo: pressing the shortcut slides the desktop away to reveal a terminal">

</p>

  

Press your shortcut and the desktop slides away to reveal a persistent terminal.

Press it again or click away to hide the terminal and get back to your desktop.

  

---

  

## Requirements

  

-  **KDE Plasma 6** on a **Wayland** session. 

> Termite will not work on Plasma 5, GNOME, or an X11 session.

 ## Download

Check out the [Releases](https://github.com/itheus/termite/releases) page

## Run it

  
Use [GearLever](https://github.com/mijorus/gearlever) to integrate the AppImage on your system automatically. 
<h5> Alternatively </h5>
Download the AppImage, make it executable, and start the background utility:

```bash

chmod  +x  Termite-0.6.0-x86_64.AppImage

./Termite-0.6.0-x86_64.AppImage  --runtime

```  

Press `Super+M` to open the terminal.

> With no arguments the AppImage opens the settings window instead, which is what the app icon does. You can launch an instance through that panel as well.

## Shortcuts

| Action | Default |
|--|--|
| Toggle the terminal | `Super + M` |
| Open Settings | `Ctrl + Super + M` |
  

 
## Build from source

  

Fedora dependencies:

  

```bash

sudo  dnf  install  cmake  ninja-build  gcc-c++  spectacle  \

qt6-qtbase-devel qt6-qtwayland-devel \

kf6-kglobalaccel-devel  kf6-kwindowsystem-devel  \

layer-shell-qt-devel qtermwidget-devel

```

  

```bash

cmake  -S  .  -B  build  -G  Ninja  -DCMAKE_BUILD_TYPE=Release

cmake  --build  build

./build/termite  --runtime

```
