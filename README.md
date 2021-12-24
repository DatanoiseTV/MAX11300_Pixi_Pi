
This is a userland application which demonstrates how to use the MAX11300
mixed-signal chip with a Raspberry Pi.

It launches an OSC server on port 7770.

It allows for setting each of the 20 I/O pins of the MAX11300 to be
analog input, output, etc. You can set the voltage ranges per I/O.

Note that due to the nature of SDL being used for threading and libbcm2835
is rather slow, the ADC sample rate is not at 100ksps.

If you are using this in a Eurorack context, this means that you can process
CV, but not audio rate signals. It might be possible to tune it to run at
higher speeds.


## Prerequisites

* libsdl2
* liblo
* libbcm2835

You can get libsdl and liblo on a Debian/Ubuntu System by using
```
$ sudo apt install -y libsdl2-dev liblo-dev
```

You will need to obtain libbcm2835 manually from the libbcm2835 page
and compile and install it manually.

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

Send a message with *int* channel and *float* value (between 0.0 and 1.0) to /cv via OSC (type "if")