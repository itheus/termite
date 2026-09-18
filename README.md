
<p  align="center">

<img align=center src="https://i.postimg.cc/63Z8dwB5/icon.png"  width="120"  alt="Termite icon">

</p>

  

<h1  align="center">Termite</h1>

  

<p  align="center"><strong>The tiny terminal that lives under your KDE desktop.</strong></p>

  

<p  align="center">

<img  src="https://i.postimg.cc/P5fv0BMh/demo.gif"  width="720"  alt="Termite demo: pressing the shortcut slides the desktop away to reveal a terminal">

</p>

  

Press a shortcut and your desktop slides away to reveal a persistent terminal.

Press it again or click away to hide the terminal and get back to your desktop.

  

---

  

## Requirements

  

-  **KDE Plasma 6** on a **Wayland** session. 

> Termite will not work on Plasma 5, GNOME, or an X11 session.

  

## Run it

  

Download the AppImage, make it executable, and start the background utility:

  

```bash

chmod  +x  Termite-0.6.0-x86_64.AppImage

./Termite-0.6.0-x86_64.AppImage  --runtime

```

  

That's it — press `Super+M` to open the terminal.

  

With no arguments the AppImage opens the settings window instead, which is what

the app icon does. To get an application-menu icon, integrate the AppImage with

your usual tool (e.g. Gear Lever). Termite does not install itself.

  

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