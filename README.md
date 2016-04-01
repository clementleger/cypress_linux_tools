
# DISCLAIMER

This work is absolutely **not related** to cypress in any way.
I am not to be liable for direct, indirect or consequential damages of your devices.

## Introduction

This repository holds an attempt to port some tools of cypress under Linux.
It allows to compile and upload application to some cypress boards.
It has only been successfully tested with a CY8CKIT-049 42xx kit.
(http://www.cypress.com/documentation/development-kitsboards/psoc-4-cy8ckit-049-4xxx-prototyping-kits)

The prerequisites are the following:
   - `gengetopt`
   - `make`
   - `wine` (to run `cyelftool` when using makefile)
   - An arm toolchain (arm-none-eabi-)

In order to build and install these utilities:

```
make all
sudo make install
```

## Host bootloader

The host bootloader has been replicating using cypress source code for host bootloader.
The Cypress Host Bootloader Tool is available as a command line utility and support various command
To build it, the `make` command should be sufficient.

### Usage

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

## iHex to cyacd format

Thanks from https://github.com/gv1/hex2cyacd, the format is well explained and it was possible to write a C tool.
ihex2cyacd is a an utility to create cyacd files from ihex files.
Using the Makefile.cypress will hide the usage.

### Usage

```

  -h, --help                 Print help and exit
  -V, --version              Print version and exit
  -i, --input=STRING         Input ihex file
  -b, --bootloader_size=INT  Bootloader text size file
  -o, --output=STRING        Output cyacd file
  -c, --cpu=ENUM             CPU type  (possible values="CY8C41", "CY8C42"
                               default=`CY8C42')
```

## Makefiles

A Makefile.cypress file is available in the repository in order to easily compile cydsn projects.
Two variable are required to be set which are PSOC_CREATOR_DIR and PROJECT_DIR.
Additionnally, An ARM toolchain is necessary. Code Sourcery ones are sufficient.
The easiest way to use it is to create a Makefile with the following content

```
PSOC_CREATOR_DIR := Path_to_PSoC_Creator
PROJECT_DIR := Path_to_project.cydsn

include Makefile.cypress

```
`cyelftool` is still used though wine (not reversed yet).

Then type `make` to compile the application.
Note that some old generated files may be outdated and can't compile.
This typically happens when you change the name of a PSoC component and rebuild the application.
Simply remove these files in order to compile again.

The output files will be in the `build` directory.
The final cyacd file can be flashed using `cyhostboot`.


## Notes

The `cyelftool` cypress utility has not yet been reversed.
However thanks to wine, this utility can be used under Linux.
