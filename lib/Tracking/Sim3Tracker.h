/**
* This file is part of LSD-SLAM.
*
* Copyright 2013 Jakob Engel <engelj at in dot tum dot de> (Technical University of Munich)
* For more information see <http://vision.in.tum.de/lsdslam>
*
* LSD-SLAM is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* LSD-SLAM is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with LSD-SLAM. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include <opencv2/core/core.hpp>
#include "util/DenseDepthTrackerSettings.h"
#include "util/EigenCoreInclude.h"
#include "util/SophusUtil.h"
#include "util/Configuration.h"
#include "Tracking/LGSX.h"


namespace lsd_slam
{

template< int __LEVELS > class _TrackingRef;
typedef _TrackingRef<PYRAMID_LEVELS> TrackingReference;

class Frame;


struct Sim3ResidualStruct
{
	float sumResD;
	float sumResP;
	int numTermsD;
	int numTermsP;

	float meanD;
	float meanP;
	float mean;

	inline Sim3ResidualStruct()
	{
		meanD = 0;
		meanP = 0;
		mean = 0;
		numTermsD = numTermsP = sumResD = sumResP = 0;
	}
};

struct Sim3TrackerDebugImages {

	Sim3TrackerDebugImages() = delete;
	Sim3TrackerDebugImages( const Sim3TrackerDebugImages & ) = delete;

	Sim3TrackerDebugImages( const ImageSize &imgSize );

	// debug images
	cv::Mat debugImageResiduals;
	cv::Mat debugImageWeights;
	cv::Mat debugImageSecondFrame;
	cv::Mat debugImageOldImageSource;
	cv::Mat debugImageOldImageWarped;
	cv::Mat debugImageExternalWeights;
	cv::Mat debugImageDepthResiduals;
	cv::Mat debugImageScaleEstimation;

	cv::Mat debugImageHuberWeight;
	cv::Mat debugImageWeightD;
	cv::Mat debugImageWeightP;
	cv::Mat debugImageWeightedResP;
	cv::Mat debugImageWeightedResD;

};


class Sim3Tracker
{
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	// int width, height;

	// camera matrix
	// Eigen::Matrix3f K, KInv;
	// float fx,fy,cx,cy;
	// float fxi,fyi,cxi,cyi;

	Matrix7x7 lastSim3Hessian;

	DenseDepthTrackerSettings<PYRAMID_LEVELS> settings;



	float* buf_warped_residual;
	float* buf_warped_weights;
	float* buf_warped_dx;
	float* buf_warped_dy;
	float* buf_warped_x;
	float* buf_warped_y;
	float* buf_warped_z;

	float* buf_d;
	float* buf_residual_d;
	float* buf_idepthVar;
	float* buf_warped_idepthVar;
	float* buf_weight_p;
	float* buf_weight_d;

	// for debug only (second-pass)
	float* buf_weight_Huber;
	float* buf_weight_VarP;
	float* buf_weight_VarD;

	int buf_warped_size;


	Sim3Tracker( const ImageSize &sz );
	Sim3Tracker(const Sim3Tracker&) = delete;
	Sim3Tracker& operator=(const Sim3Tracker&) = delete;
	~Sim3Tracker();

	Sim3 trackFrameSim3(
			const std::shared_ptr<TrackingReference> &reference,
			const std::shared_ptr<Frame> &frame,
			const Sim3& frameToReference_initialEstimate,
			int startLevel, int finalLevel);


	float pointUsage;
	float lastResidual, lastDepthResidual, lastPhotometricResidual;
	float lastResidualUnweighted, lastDepthResidualUnweighted, lastPhotometricResidualUnweighted;


	float affineEstimation_a;
	float affineEstimation_b;

	bool diverged;


private:

	const ImageSize &_imgSize;

	Sim3TrackerDebugImages _debugImages;

	void calcSim3Buffers(
			const std::shared_ptr<TrackingReference> &reference,
			const std::shared_ptr<Frame> &frame,
			const Sim3& referenceToFrame,
			int level,
			bool plotWeights = false);
#if defined(ENABLE_SSE)
	void calcSim3BuffersSSE(
			const std::shared_ptr<TrackingReference> &reference,
			const std::shared_ptr<Frame> &frame,
			const Sim3& referenceToFrame,
			int level,
			bool plotWeights = false);
#endif
#if defined(ENABLE_NEON)
	void calcSim3BuffersNEON(
			const std::shared_ptr<TrackingReference> &reference,
			const std::shared_ptr<Frame> &frame,
			const Sim3& referenceToFrame,
			int level,
			bool plotWeights = false);
#endif






	Sim3ResidualStruct calcSim3WeightsAndResidual(
			const Sim3& referenceToFrame);
#if defined(ENABLE_SSE)
	Sim3ResidualStruct calcSim3WeightsAndResidualSSE(
			const Sim3& referenceToFrame);
#endif
#if defined(ENABLE_NEON)
	Sim3ResidualStruct calcSim3WeightsAndResidualNEON(
			const Sim3& referenceToFrame);
#endif



	void calcSim3LGS(LGS7 &ls7);
#if defined(ENABLE_SSE)
	void calcSim3LGSSSE(LGS7 &ls7);
#endif
#if defined(ENABLE_NEON)
	void calcSim3LGSNEON(LGS7 &ls7);
#endif



	void calcResidualAndBuffers_debugStart();
	void calcResidualAndBuffers_debugFinish(int w);


	// used for image saving
	int iterationNumber;

	float affineEstimation_a_lastIt;
	float affineEstimation_b_lastIt;
};

}
