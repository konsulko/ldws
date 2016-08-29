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

	`cmake .`
	`make`

Run
---

First copy the example conf file to the top level directory as ldws.conf:

	`cp examples/ldws-example.conf ldws.conf`

and execute LDWS:

	`./ldws`

License
-------

Most of the code is Apache licensed (LICENSE) while the algorithm used in
the LaneDetector class is MIT licensed (LICENSE.MIT).

TODO
----

* Add lane departure detection and alerts
