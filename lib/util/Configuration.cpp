#include "Configuration.h"

#include "DataStructures/FrameMemory.h"

namespace lsd_slam {

Configuration &Conf() {
        static Configuration TheInstance;
        return TheInstance;
}

Configuration::Configuration()
        : runRealTime(true), doDepth(NO_STEREO), stopOnFailedRead(true),
        SLAMEnabled(true), doKFReActivation(true), doMapping(true),
        continuousPCOutput(true),

        debugDisplay(0), displayDepthMap(false), displayGradientMap(false),
        onSceenInfoDisplay(true), dumpMap(false), doFullReConstraintTrack(false),

        doSubpixelStereo(true), doLeftRightStereo(false),

        minVirtualBaselineLength(0.001), suppressLSDPoints(false),

        minEplLengthCrop(3.0f), maxEplLengthCrop(30.0f),

        gradientSampleDistance(1.0f), lsdFrame("camera_left"), globalFrame("map"),

        doImageSaturation(false), saturationAlpha(1.0), saturationBeta(1),
        useRectificationFrame(false), displayInputFusedImage(false),
        minAbsGradCreate(10), minAbsGradDecrease(10),
        displayInputImage(false), pointcloudSize(1.0), print(), plot() {
}

const ImageSize &Configuration::setSlamImageSize(const ImageSize &sz) {
        CHECK(sz.width % 16 == 0 && sz.height % 16 == 0)
                << "SLAM image dimensions must be multiples of 16! Please crop your "
                "images / video accordingly.";
        CHECK(sz.width != 0 && sz.height != 0) << "Height or width set to zero!";

        slamImageSize = sz;
        return slamImageSize;
}

//==
Configuration::PrintConfiguration::PrintConfiguration()
        : threadingInfo(true), memoryDebugInfo(false), trackingIterationInfo(false),
        observeStatistics(false), observePurgeStatistics(false),
        propagationStatistics(false), fillHolesStatistics(false),
        regularizeStatistics(false), lineStereoStatistics(false),
        lineStereoFails(false) {
        ;
}

//==
Configuration::PlotConfiguration::PlotConfiguration()
        : doWaitKey(-1), debugStereo(false) {
        ;
}

} // namespace lsd_slam
