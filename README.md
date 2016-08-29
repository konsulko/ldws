LDWS
====

Simple OpenCV/C++ Lane Departure Warning System with GPGPU support

Dependencies
------------
	* pkg-config
	* TCLAP >= 1.2.0
	* libconfig++ >=1.5
	* OpenCV >= 3.0

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

License
-------

Most of the code is Apache licensed (LICENSE) while the algorithm used in
the LaneDetector class is MIT licensed (LICENSE.MIT).

TODO
----

* Add lane departure detection and alerts
