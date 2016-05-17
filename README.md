# LSD-SLAM: Large-Scale Direct Monocular SLAM

CI Status: [![Build Status](https://travis-ci.org/amarburg/lsd_slam.svg)](https://travis-ci.org/amarburg/lsd_slam)

This fork started from [Thomas Whelan's fork](https://github.com/mp3guy/lsd_slam) which "relieves the user of the horrors of a ROS dependency and uses the much nicer lightweight [Pangolin](https://github.com/stevenlovegrove/Pangolin) framework instead."

Here is Jakob's original description:

> LSD-SLAM is a novel approach to real-time monocular SLAM. It is fully direct
> (i.e. does not use keypoints / features) and creates large-scale,
> semi-dense maps in real-time on a laptop. For more information see
> [http://vision.in.tum.de/lsdslam](http://vision.in.tum.de/lsdslam)
> where you can also find the corresponding publications and Youtube videos, as well as some
> example-input datasets, and the generated output as rosbag or .ply point cloud.

This repo contains my experiments with LSD-SLAM, for performance, functionality
and structure.   As of March 2016, it diverges significantly from either Jakob
or Thomas's branches in structure (I refactored as a way of learning the code),
but not significantly in terms of functionality (except for all the ways in which
I've broken it in the refactoring).   

**master**  is my working / stable-ish branch.   **aaron_dev** is my
**really unstable** branch.   Other branches are for hardware- or feature-specific
development
although in the long run I try to merge those functionalities into master
and use CMake to turn hardware-specific elements on and off.

# 1. Quickstart


My targeted environments are Ubuntu 14.04.2, [NVidia Jetpack 2.0](https://developer.nvidia.com/embedded/jetpack) for Jetson TX1, and OS X 10.11 with Homebrew.

The most authoritative set of dependencies is stored in the [Travis CI](https://www.travis-ci.org) bootstrap files.  Do:

    .travis/install_deps_{osx,trusty}.sh
    .travis/build.sh

LSD-SLAM uses these "standard" dependencies: OpenCV 2.4 (with nonfree if you want FABMAP),
[TCLAP](http://tclap.sourceforge.net/), Boost, Eigen.   

LSD-SLAM also uses these "non-standard" dependencies: Pangolin, g2o,
[g3log](https://github.com/KjellKod/g3log), and optionally the [StereoLabs
Zed](https://www.stereolabs.com/) SDK and [Google
Snappy](https://github.com/google/snappy) for file compression.

By default, LSD-SLAM It will use CMake ExternalProjects to build each of these dependencies automatically.   
This is the most repeatable path but will be slow to re-compile as it will update each dependency every time.

Set the appropriate CMake variable `BUILD_LOCAL_* = OFF` to disable local building.

**CMake will not resolve these dependencies correctly when building in parallel ('make -j'). On the first build, use just 'make'.   Once the dependencies have been made (they should be reasonably stable), you can 'make -j' when rebuilding just LSD-SLAM.**

If you want to build G2O, Pangolin, etc. yourself, see the `cmake/Build*` files for the CMake flags I used.

See also [doc/CommonProblems.md](doc/CommonProblems.md)

# 4. Running

Supports directories or sets of raw PNG images. For example, you can down any dataset from [here](http://vision.in.tum.de/lsdslam) in PNG format, and run like;

    ./LSD --calib datasets/LSD_machine/cameraCalibration.cfg  datasets/LSD_machine/images/

I've started to record my performance results in [doc/Performance.md](doc/Performance.md)

# 5. Related Papers

* **LSD-SLAM: Large-Scale Direct Monocular SLAM**, *J. Engel, T. Schöps, D. Cremers*, ECCV '14

* **Semi-Dense Visual Odometry for a Monocular Camera**, *J. Engel, J. Sturm, D. Cremers*, ICCV '13

# 6. License

LSD-SLAM is licensed under the GNU General Public License Version 3 (GPLv3), see http://www.gnu.org/licenses/gpl.html.
