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