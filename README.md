
# DISCLAIMER

This work is absolutely **not related** to cypress in any way.

## Introduction

This repository holds an attempt to port some tools of cypress under Linux.
The host bootloader has been replicating using cypress source code for host bootloader.
It has only been successfully tested with a CY8CKIT-049 42xx kit.
(http://www.cypress.com/documentation/development-kitsboards/psoc-4-cy8ckit-049-4xxx-prototyping-kits)

The prerequisites are the following:
   - `gengetopt`
   - `make`


## Host bootloader

The Cypress Host Bootloader Tool is available as a command line utility and support various command
To build it, the `make` command should be sufficient.

## Usage

```
  -h, --help           Print help and exit
  -V, --version        Print version and exit
  -b, --baudrate=INT   Bootloader baudrate  (default=`115200')
  -f, --file=STRING    cyacd file to flash
  -s, --serial=STRING  Serial port to use  (default=`/dev/ttyACM0')
  -a, --app_id=INT     Application id to use (0 for no change, or 1 or 2)
                         (default=`0')

 Group: Action
  Action to perform (default=`program`)
  -p, --program        Program the file
  -e, --erase          Erase memory
  -v, --verify         Verify file
```
