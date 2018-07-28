# fvwm95
---
This is a fork of fvwm95 meant to be compilable on and play nice with newer Unix-like operating systems.

It is based off the code from the source package in Debian Sarge, slightly modified so that it compiles without errors on modern OSes, and with a more sane configuration to adapt to modern day needs.

![Reference screenshot](/screenshot.png?raw=true "Reference screenshot")

## How to build
The following packages are needed in order to build fvwm95:

On Debian/Ubuntu:
* xorg
* build-essential
* libx11-dev
* libxt-dev
* libxext-dev
* libxpm-dev
* libreadline-dev
* libxmu-headers
* bison
* flex
* autoconf

On Arch:
* xorg
* xorg-xinit
* xbitmaps
* libx11
* libxt
* libxext
* libxpm
* readline
* libxmu
* bison
* flex
* autoconf

The following packages are not necessary to build fvwm95 itself, but
they are recommended to have:

On Debian/Ubuntu and Arch:
* xscreensaver
* nitrogen
* thunar
* xterm
* lxappearance
* pulseaudio
* pavucontrol

## Actually installing
After all the dependencies are satisfied, configure, build, and
install fvwm95 by running:
```
./configure --prefix=/usr/local
make
sudo make install
```

In order to start fvwm95 with xinit, add the following to your
`~/.xinitrc` file:
```
exec fvwm95
```

To start it, simply type:
```
startx
```

Enjoy!
