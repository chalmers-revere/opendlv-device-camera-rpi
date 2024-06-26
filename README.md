Moved to https://git.opendlv.org.

## OpenDLV Microservice to interface with RPi cameras

This repository provides source code to interface with an RPi camera
for the OpenDLV software ecosystem. This microservice provides the captured frames
in two separate shared memory areas, one for a picture in [I420 format](https://wiki.videolan.org/YUV/#I420)
and one in ARGB format.

This product includes software developed by the Ava group of the University of Cordoba.

[![Build Status](https://travis-ci.org/chalmers-revere/opendlv-device-camera-rpi.svg?branch=master)](https://travis-ci.org/chalmers-revere/opendlv-device-camera-rpi) [![License: GPLv3](https://img.shields.io/badge/license-GPL--3-blue.svg
)](https://www.gnu.org/licenses/gpl-3.0.txt)


## Table of Contents
* [Dependencies](#dependencies)
* [Usage](#usage)
* [Build from sources on the example of Ubuntu 16.04 LTS](#build-from-sources-on-the-example-of-ubuntu-1604-lts)
* [License](#license)


## Dependencies
You need a C++14-compliant compiler to compile this project. The following
dependency is shipped as part of the source distribution:

* [libcluon](https://github.com/chrberger/libcluon) - [![License: GPLv3](https://img.shields.io/badge/license-GPL--3-blue.svg
)](https://www.gnu.org/licenses/gpl-3.0.txt)

The following dependencies are downloaded and installed during the Docker-ized build:
* [libyuv](https://chromium.googlesource.com/libyuv/libyuv/+/master) - [![License: BSD 3-Clause](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause) - [Google Patent License Conditions](https://chromium.googlesource.com/libyuv/libyuv/+/master/PATENTS)
* [raspicam](https://github.com/cedricve/raspicam) - [![License: BSD 3-Clause](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)


## Usage
This microservice is created automatically on changes to this repository via Docker's public registry for:
* [armhf](https://hub.docker.com/r/chalmersrevere/opendlv-device-camera-rpi-armhf/tags/)

To run this microservice using our pre-built Docker image to open an RPi camera, simply start it as follows:

```
docker run --rm -ti --init --ipc=host -v /tmp:/tmp -e DISPLAY=$DISPLAY chalmersrevere/opendlv-device-camera-rpi-armhf:v0.0.6 --width=640 --height=480 --freq=20 --verbose
```


## Build from sources on the example of Ubuntu 16.04 LTS
To build this software, you need cmake, C++14 or newer, libx11-dev, and make.
Having these preconditions, just run `cmake` and `make` as follows:

```
mkdir build && cd build
cmake -D CMAKE_BUILD_TYPE=Release ..
make && make test && make install
```


## License

* This project is released under the terms of the GNU GPLv3 License

