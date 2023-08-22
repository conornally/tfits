# tfits

Version=1.1.2

Fits image displayed in the terminal..in ASCII. Because what better way to view those beautiful high resolution images from Hubble or JWST than in ascii.

It is actually useful in the situation where you are logged onto a system with no running X session.

```
usage: tfits [-eHhirvVx] [-a character set] [-n nHDU] [-p power] [-s stretch] file.fits[nHDU]
  -a character set : ascii char set (default  .,;!o%0#)
  -e               : display extension information
  -H               : display header file
  -h               : print help message
  -i               : invert ascii char set
  -n nHDU          : nHDU to print, (synonymous with file[nHDU])
  -p power         : stretch to exponent p
  -r               : rotate image
  -s stretch       : sin, asin, sinh
  -v verbose       : print verbose outputs
  -V               : print version
  -x               : don't ask to move to next EXTENSION
```

## Compilation

Requirements: [libcfitsio](https://heasarc.gsfc.nasa.gov/fitsio/)

There is a simple makefile but really it only has a single gcc command. 

```bash
gcc -O2 -Wall -o tfits tfits.c -lcfitsio -lm
```

OR

```bash
make
make install
```




## TODO

Include table outputs?

> Rotate 													
> Double scale x to account for non square terminal- font 	
> Region/Catalog overlays 

### Tables
	>>> ability to list all column names
	>>> ability to turn the entrie thing to ascii
	>>> show just certain columns
	>>> show everything in "less" style viewer
		|--> hjkl and arrows
		|--> keep current cursor position
		|--> move around table 
		|--> search?
	>>> search table
		|--> crop out good/bad
	

