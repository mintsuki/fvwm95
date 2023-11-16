# fvwm95

This is a fork of fvwm95 meant to be compilable on and play nice with newer Unix-like operating systems.

It is based off the code from the source package in Debian Sarge, slightly modified so that it compiles without errors on modern OSes, and with a more sane configuration to adapt to modern day needs.

![Reference screenshot](/screenshot.png?raw=true "Reference screenshot")

## Build prerequisites
The following packages are needed in order to build fvwm95:

On Debian/Ubuntu:
```sh
sudo apt-get install build-essential autoconf automake pkg-config xorg xinit xbitmaps libx11-dev libxt-dev libxext-dev libxpm-dev libreadline-dev libxmu-headers
```

On Arch:
```sh
sudo pacman -S --needed base-devel autoconf automake pkgconf xorg xorg-xinit xbitmaps libx11 libxt libxext libxpm readline libxmu
```

The following packages are not necessary to build fvwm95 itself, but
they are nice to have with the default config:

* xscreensaver
* nitrogen
* thunar
* xterm
* lxappearance
* pulseaudio
* pavucontrol

## Building
After all the dependencies are satisfied - configure, build, and
install (into /usr/local - change if wanted) fvwm95 by running:
```
./bootstrap
./configure --prefix=/usr/local
make
sudo make install
```

## Running

### xinit
In order to start fvwm95 with xinit, add the following to your
`~/.xinitrc` file:
```
exec fvwm95
```

To start it, simply type:
```
startx
```
