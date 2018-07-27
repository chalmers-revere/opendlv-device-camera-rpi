/*
 * Copyright (C) 2018  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cluon-complete.hpp"

#include <raspicam/raspicam.h>

#include <X11/Xlib.h>
#include <libyuv.h>

#include <cstdint>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("cid")) ||
         (0 == commandlineArguments.count("width")) ||
         (0 == commandlineArguments.count("height")) ||
         (0 == commandlineArguments.count("freq")) ) {
        std::cerr << argv[0] << " interfaces with the Raspberry PI camera to provide the captured image in two shared memory areas: one in I420 format and one in ARGB format." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --width=<width> --height=<height> --freq=<FPS> [--name.i420=<unique name for the shared memory in I420 format>] [--name.argb=<unique name for the shared memory in ARGB format>] [--verbose]" << std::endl;
        std::cerr << "         --cid:       CID of the OD4Session to send h264 frames" << std::endl;
        std::cerr << "         --name.i420: name of the shared memory for the I420 formatted image; when omitted, video0.i420 is chosen" << std::endl;
        std::cerr << "         --name.argb: name of the shared memory for the ARGB formatted image; when omitted, video0.argb is chosen" << std::endl;
        std::cerr << "         --width:     desired width of a frame" << std::endl;
        std::cerr << "         --height:    desired height of a frame" << std::endl;
        std::cerr << "         --freq:      desired frame rate" << std::endl;
        std::cerr << "         --verbose:   display captured image" << std::endl;
        std::cerr << "Example: " << argv[0] << " --width=640 --height=480 --freq=20 --verbose" << std::endl;
    } else {
        const std::string NAME_I420{(commandlineArguments["name.i420"].size() != 0) ? commandlineArguments["name.i420"] : "video0.i420"};
        const std::string NAME_ARGB{(commandlineArguments["name.argb"].size() != 0) ? commandlineArguments["name.argb"] : "video0.argb"};
        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
        const float FREQ{static_cast<float>(std::stof(commandlineArguments["freq"]))};
        if ( !(FREQ > 0) ) {
            std::cerr << "[opendlv-device-camera-rpi]: freq must be larger than 0; found " << FREQ << "." << std::endl;
            return retCode;
        }

        const bool VERBOSE{commandlineArguments.count("verbose") != 0};

        raspicam::RaspiCam camera;
        camera.setWidth(WIDTH);
        camera.setHeight(HEIGHT);
        if (!camera.open()) {
            std::cerr << "[opendlv-device-camera-rpi]: Could not open camera." << std::endl;
            return retCode = 1;
        }
        // Camera warm-up time.
        std::this_thread::sleep_for(std::chrono::seconds(3));

        std::unique_ptr<cluon::SharedMemory> sharedMemoryI420(new cluon::SharedMemory{NAME_I420, WIDTH * HEIGHT * 3/2});
        if (!sharedMemoryI420 || !sharedMemoryI420->valid()) {
            std::cerr << "[opendlv-device-camera-rpi]: Failed to create shared memory '" << NAME_I420 << "'." << std::endl;
            return retCode = 1;
        }

        std::unique_ptr<cluon::SharedMemory> sharedMemoryARGB(new cluon::SharedMemory{NAME_ARGB, WIDTH * HEIGHT * 4});
        if (!sharedMemoryARGB || !sharedMemoryARGB->valid()) {
            std::cerr << "[opendlv-device-camera-rpi]: Failed to create shared memory '" << NAME_ARGB << "'." << std::endl;
            return retCode = 1;
        }

        if ( (sharedMemoryI420 && sharedMemoryI420->valid()) &&
             (sharedMemoryARGB && sharedMemoryARGB->valid()) ) {
            std::clog << "[opendlv-device-camera-rpi]: Data from camera available in I420 format in shared memory '" << sharedMemoryI420->name() << "' (" << sharedMemoryI420->size() << ") and in ARGB format in shared memory '" << sharedMemoryARGB->name() << "' (" << sharedMemoryARGB->size() << ")." << std::endl;

            // Accessing the low-level X11 data display.
            Display* display{nullptr};
            Visual* visual{nullptr};
            Window window{0};
            XImage* ximage{nullptr};
            if (VERBOSE) {
                display = XOpenDisplay(NULL);
                visual = DefaultVisual(display, 0);
                window = XCreateSimpleWindow(display, RootWindow(display, 0), 0, 0, WIDTH, HEIGHT, 1, 0, 0);
                sharedMemoryARGB->lock();
                {
                    ximage = XCreateImage(display, visual, 24, ZPixmap, 0, sharedMemoryARGB->data(), WIDTH, HEIGHT, 32, 0);
                }
                sharedMemoryARGB->unlock();
                XMapWindow(display, window);
            }

            // Interface to a running OpenDaVINCI session (ignoring any incoming Envelopes).
            cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

            while (!od4.isRunning()) {
                camera.grab();

                sharedMemoryI420->lock();
                {
                    camera.retrieve(reinterpret_cast<unsigned char*>(sharedMemoryI420->data()), raspicam::RASPICAM_FORMAT_YUV420);
                }
                sharedMemoryI420->unlock();

                // Transform I420 frame to ARGB frame.
                sharedMemoryARGB->lock();
                {
                    libyuv::I420ToARGB(reinterpret_cast<uint8_t*>(sharedMemoryI420->data()), WIDTH,
                                       reinterpret_cast<uint8_t*>(sharedMemoryI420->data()+(WIDTH * HEIGHT)), WIDTH/2,
                                       reinterpret_cast<uint8_t*>(sharedMemoryI420->data()+(WIDTH * HEIGHT + ((WIDTH * HEIGHT) >> 2))), WIDTH/2,
                                       reinterpret_cast<uint8_t*>(sharedMemoryARGB->data()), WIDTH * 4, WIDTH, HEIGHT);

                    if (VERBOSE) {
                        XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 0, 0, WIDTH, HEIGHT);
                    }
                }
                sharedMemoryARGB->unlock();

                sharedMemoryARGB->notifyAll();
                sharedMemoryI420->notifyAll();
            }
            if (VERBOSE) {
                XCloseDisplay(display);
            }
        }
        retCode = 0;
    }
    return retCode;
}
