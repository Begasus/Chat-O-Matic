# Cardie
![GSoC 2021](https://img.shields.io/badge/GSoC-2021-green.svg)

A multi-protocol chat program based on [Caya](https://github.com/Augustolo/Caya).

![Screenshot](data/screenshots/update-3.png)

## Building
You can make Cardie and its protocols with:

`$ make`

Or one-by-one:

`$ make libs; make app; make protocols`

Cardie itself requires the `expat_devel` package, the XMPP protocol requires
`gloox_devel`, and the libpurple add-on requires `libpurple_devel` and
`glib2_devel`.

The (provisional) IRC protocol has to be built specifically:

`$ make -f protocols/irc/Makefile`

## License
Cardie itself is under the MIT license, but licenses vary for the included
libraries and add-ons.

The `xmpp` and `purple` add-ons are under the GPLv2+, and `irc` the MIT license.

`libsupport` is under the MIT license, though containing some PD code.
`librunview` contains code from Vision, and is under the MPL.
`libinterface` is under the MIT license.
