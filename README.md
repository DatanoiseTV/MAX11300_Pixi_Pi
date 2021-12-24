
This is a userland application which demonstrates how to use the MAX11300
mixed-signal chip with a Raspberry Pi.

It allows for setting each of the 20 I/O pins of the MAX11300 to be
analog input, output, etc. You can set the voltage ranges per I/O.

Note that due to the nature of SDL being used for threading and libbcm2835
is rather slow, the ADC sample rate is not at 100ksps.

If you are using this in a Eurorack context, this means that you can process
CV, but not audio rate signals. It might be possible to tune it to run at
higher speeds.

## Building

```
$ mkdir build
$ cd build
$ cmake ..
$ make -j4
```

The resulting binary *main* is at ../bin

## Running

```
$ sudo ../bin/main
```