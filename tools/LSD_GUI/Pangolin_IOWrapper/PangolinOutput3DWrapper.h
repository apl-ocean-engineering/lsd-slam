/*
 * PangolinOutput3DWrapper.h
 *
 *  Created on: 17 Oct 2014
 *      Author: thomas
 */

#pragma once

#include "IOWrapper/Output3DWrapper.h"
#include "Keyframe.h"
#include "LSD_GUI/GUI.h"
#include "util/Configuration.h"

namespace lsd_slam
{

class Frame;
class KeyFrameGraph;

struct GraphConstraint
{
    int from;
    int to;
    float err;
};

class PangolinOutput3DWrapper : public Output3DWrapper
{
    public:
        PangolinOutput3DWrapper( const Configuration &conf, GUI & gui);
        virtual ~PangolinOutput3DWrapper();

        virtual void publishKeyframeGraph(const shared_ptr<KeyFrameGraph> &graph);

        // publishes a keyframe. if that frame already existis, it is overwritten, otherwise it is added.
        virtual void publishKeyframe(const Frame::SharedPtr &f);

        virtual void updateDepthImage(unsigned char * data);

        // published a tracked frame that did not become a keyframe (i.e. has no depth data)
        virtual void publishTrackedFrame(const Frame::SharedPtr &f);

        // publishes graph and all constraints, as well as updated KF poses.
        virtual void publishTrajectory(std::vector<Eigen::Matrix<float, 3, 1>> trajectory, std::string identifier);

        virtual void publishTrajectoryIncrement(Eigen::Matrix<float, 3, 1> pt, std::string identifier);

        virtual void publishDebugInfo(Eigen::Matrix<float, 20, 1> data);

        virtual void publishPose( const Sophus::Sim3f &pose );


        int publishLvl;

    private:
        const Configuration &_conf;

        GUI & _gui;
};
}
