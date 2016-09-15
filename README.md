LDWS
====

Simple OpenCV/C++ Lane Departure Warning System with GPGPU support

Dependencies
------------

	* pkg-config
	* TCLAP >= 1.2.0
	* libconfig++ >=1.4
	* OpenCV >= 3.1
		- cmake options: *-D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local -D WITH_TBB=ON -D BUILD_NEW_PYTHON_SUPPORT=ON -D WITH_V4L=ON -D INSTALL_C_EXAMPLES=ON -D INSTALL_PYTHON_EXAMPLES=ON -D BUILD_EXAMPLES=ON -D WITH_QT=ON -D WITH_OPENGL=ON -D ENABLE_FAST_MATH=1 -D CUDA_FAST_MATH=1 -D WITH_CUBLAS=1*
		- nvidia driver 352.79+
		- nvidia-cuda-toolkit 7.5.18+
			* gcc 4.8 or 4.9
		- libavcodec/libavformat/libswscale
		- gstreamer
		- qt4

Build
-----

	cmake .
	make

Run
---

Execute LDWS:

	./ldws --config-file examples/road-dual.conf

or

	./ldws --config-file examples/road-single.conf

Enable OpenCL acceleration by adding

	--enable-opencl

Enable CUDA acceleration by adding

	--enable-cuda

Usage
-----

```
   ldws  [-c <filename>] [-v] [-f] [-w] [-i] [-d] [-o] [-u] [--]
           [--version] [-h]


Where: 

   -c <filename>,  --config-file <filename>
     Configuration file name

   -v,  --verbose
     Verbose messages

   -f,  --frame-dump
     Dump last captured frame (and optionally intermediate image frames) to
     file

   -w,  --write-video
     Write video to a file

   -i,  --display-intermediate
     Display intermediate processing steps

   -d,  --disable-display
     Disable video display

   -o,  --enable-opencl
     Enable OpenCL support

   -u,  --enable-cuda
     Enable CUDA support

   --,  --ignore_rest
     Ignores the rest of the labeled arguments following this flag.

   --version
     Displays version information and exits.

   -h,  --help
     Displays usage information and exits.
```

License
-------

Most of the code is Apache licensed (LICENSE) while the algorithm used in
the LaneDetector class is MIT licensed (LICENSE.MIT).
