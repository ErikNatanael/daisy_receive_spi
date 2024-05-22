# daisy_receive_spi

## Set up daisy libs

```
git submodule update --init --recursive
echo "building DaisySP . . ."
cd "DaisySP" ; make clean ; make | grep "warning:r\|error" ;
echo "done."

cd ../libDaisy
echo "building libDaisy . . ."
make clean ; make | grep "warning:r\|error" ;
echo "done."
```

## Building

cd into the directory of the program you want to build and run

```
make clean; make
```

## Uploading

Put Daisy into DFU mode by pressing BOOT, then RESET, then letting go of RESET and then of BOOT.

```
make program-dfu
```

Or build and upload in one:

```
make clean; make; make program-dfu
```


## Docs

https://electro-smith.github.io/libDaisy/md_doc_2md_2__a8___getting-_started-_s_p_i.html

https://github.com/electro-smith/libDaisy/tree/master/examples/SPI

https://forum.electro-smith.com/t/spi-examples/4973


### Daisy pinout
https://github.com/electro-smith/DaisyWiki/wiki/2.-Daisy-Seed-Pinout

# Connecting pins

The two chips need to be connected by GND (otherwise you need optocouplers) and a resistor on the signal pins to avoid driving the other chip through the data pins. This post recommends 3.3kOhm closest to the side receiving input.
https://forum.arduino.cc/t/will-spi-work-between-two-systems-with-different-power-supplies/168680/6


# ESP32 Websocket

https://github.com/dvarrel/AsyncTCP