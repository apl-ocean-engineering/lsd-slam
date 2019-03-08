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

#include <boost/thread/shared_lock_guard.hpp>

#include "TrackingThread.h"

#include "SlamSystem.h"

#include "DataStructures/KeyFrame.h"
#include "Tracking/SE3Tracker.h"
// #include "Tracking/Sim3Tracker.h"
// #include "DepthEstimation/DepthMap.h"
#include "Tracking/TrackingReference.h"
// #include "util/globalFuncs.h"
#include "GlobalMapping/KeyFrameGraph.h"
#include "GlobalMapping/TrackableKeyFrameSearch.h"
// #include "GlobalMapping/g2oTypeSim3Sophus.h"
// #include "IOWrapper/ImageDisplay.h"
// #include "IOWrapper/Output3DWrapper.h"
// #include <g2o/core/robust_kernel_impl.h>
// #include "DataStructures/FrameMemory.h"
// #include "deque

#include "SlamSystem/MappingThread.h"

// for mkdir
#include <sys/types.h>
#include <sys/stat.h>

#include <g3log/g3log.hpp>

#ifdef ANDROID
#include <android/log.h>
#endif

#include "opencv2/opencv.hpp"

using namespace lsd_slam;

using active_object::Active;

TrackingThread::TrackingThread( SlamSystem &system, bool threaded )
: _system( system ),
	_perf(),
	_tracker( new SE3Tracker( Conf().slamImageSize ) ),
	//_trackingReference( new TrackingReference() ),
	_initialized( false ),
	_trackingIsGood( true ),
	_newKeyFramePending( false ),
	_thread( threaded ? Active::createActive() : NULL )
{


	// this->width = w;
	// this->height = h;
	// this->K = K;
	// trackingIsGood = true;

	// keyFrameGraph = new KeyFrameGraph();

	// createNewKeyFrame = false;

	// map =  new DepthMap( conf );

	// newConstraintAdded = false;
	//haveUnmergedOptimizationOffset = false;


	// Do not use more than 4 levels for odometry tracking
	for (int level = 4; level < PYRAMID_LEVELS; ++level)
		_tracker->settings.maxItsPerLvl[level] = 0;

	// trackingReference = new TrackingReference();
	//mappingTrackingReference = new TrackingReference();


	// if(SLAMEnabled)
	// {
	// 	trackableKeyFrameSearch = new TrackableKeyFrameSearch(keyFrameGraph,conf);
	// 	constraintTracker = new Sim3Tracker( _conf.slamImage );
	// 	constraintSE3Tracker = new SE3Tracker( _conf.slamImage );
	// 	newKFTrackingReference = new TrackingReference();
	// 	candidateTrackingReference = new TrackingReference();
	// }
	// else
	// {
	// 	constraintSE3Tracker = 0;
	// 	trackableKeyFrameSearch = 0;
	// 	constraintTracker = 0;
	// 	newKFTrackingReference = 0;
	// 	candidateTrackingReference = 0;
	// }


	// outputWrapper = 0;

	// keepRunning = true;
	// depthMapScreenshotFlag = false;
	lastTrackingClosenessScore = 0;
}


TrackingThread::~TrackingThread()
{
}


/*
void TrackingThread::trackSet( const std::shared_ptr<ImageSet> &set )
{
	trackFrame( set->refFrame() );
}
*/


void TrackingThread::trackSetImpl( const std::shared_ptr<ImageSet> &set )
{

	if( !_initialized ) {
		// Take ref frame in set as keyframe
		_currentKeyFrame = KeyFrame::Create( set->refFrame() );

		_system.keyFrameGraph()->allFramePoses.push_back( _currentKeyFrame->pose() );
		_system.addKeyFrame( _currentKeyFrame );

		_initialized = true;

		// TODO.  How to handle multi-image sets?
		return;
	}

        if(!_trackingIsGood) {
                // Prod mapping to check the relocalizer

								//!!TODO.  Fix this
                // _system._mapThread->relocalizer.updateCurrentFrame(set->refFrame());
                // _system._mapThread->pushDoIteration();

                return;
        }

        // // Are the following two calls atomic enough or should I lock before the next two lines?
        // _system.currentKeyFrame()->frameMutex.lock();
        // Frame::SharedPtr keyframe( _system.currentKeyFrame() );
        // _system.currentKeyFrame()->frameMutex.unlock();



        // if(_trackingReference->frameID != keyframe->id() || keyframe->depthHasBeenUpdatedFlag ) {
        //         LOG(DEBUG) << "Importing new tracking reference from frame " << keyframe->id();
        //         _trackingReference->importFrame( keyframe );
        //         keyframe->depthHasBeenUpdatedFlag = false;
        //         _trackingReferenceFrameSharedPT = keyframe;
        // }

        //FramePoseStruct &trackingReferencePose( *_trackingReference->keyframe->pose);

				//const FramePoseStruct::SharedPtr &trackingFramePose( _currentKeyFrame->pose() );

        // DO TRACKING & Show tracking result.
        LOG_IF(DEBUG, Conf().print.threadingInfo) << "TRACKING frame " << set->refFrame()->id() << " onto ref. " << _currentKeyFrame->id();

        SE3 frameToReference_initialEstimate;
        {
            boost::shared_lock_guard<boost::shared_mutex> lock( _system.poseConsistencyMutex );
						CHECK( _system.keyFrameGraph()->allFramePoses.size() > 0 ) << "No poses in allFramePoses";

						// LOG(DEBUG) << "Building initial estimate";
						// LOG(DEBUG) << "Tracking reference " << _currentKeyFrame->pose()->getCamToWorld().translation();
						// LOG(DEBUG) << "Last pose (" << _system.keyFrameGraph()->allFramePoses.size() << "): " << _system.keyFrameGraph()->allFramePoses.back()->getCamToWorld().translation();

						// Estimate pose from currentFrame and most recent pose
            frameToReference_initialEstimate = se3FromSim3(  _currentKeyFrame->pose()->getCamToWorld().inverse() * _system.keyFrameGraph()->allFramePoses.back()->getCamToWorld());
        }


        Timer timer;

        LOG(DEBUG) << "Start tracking...";
        SE3 newRefToFrame_poseUpdate = _tracker->trackFrame( _currentKeyFrame,
                                                            set->refFrame(),
                                                            frameToReference_initialEstimate);

        LOG(DEBUG) << "Done tracking, took " << timer.stop() * 1000 << " ms";
        _perf.track.update( timer );


        tracking_lastResidual = _tracker->lastResidual;
        tracking_lastUsage = _tracker->pointUsage;

        if(manualTrackingLossIndicated || _tracker->diverged ||
                (_system.keyFrameGraph()->keyframesAll.size() > INITIALIZATION_PHASE_COUNT && !_tracker->trackingWasGood))
        {
                LOGF(WARNING, "TRACKING LOST for frame %d (%1.2f%% good Points, which is %1.2f%% of available points; %s tracking; tracker has %s)!\n",
                                set->refFrame()->id(),
                                100*_tracker->_pctGoodPerTotal,
                                100*_tracker->_pctGoodPerGoodBad,
                                _tracker->trackingWasGood ? "GOOD" : "BAD",
                                _tracker->diverged ? "DIVERGED" : "NOT DIVERGED");

                //_trackingReference->invalidate();
                setTrackingIsBad();

								//!!TODO.  What does mapping thread do while tracking is bad?
                //_system.mapThread->pushDoIteration();
                manualTrackingLossIndicated = false;
                return;
        }
        _system.keyFrameGraph()->addFrame(set->refFrame());

        LOG_IF( DEBUG,  Conf().print.threadingInfo ) << "Publishing tracked frame";
        _system.publishTrackedFrame(set->refFrame());
        _system.publishPose(set->refFrame()->getCamToWorld().cast<float>());


        // Keyframe selection
        LOG(INFO) << "Tracked " << set->id() << " against keyframe " << _currentKeyFrame->id();
        LOG_IF( INFO, Conf().print.threadingInfo ) << _currentKeyFrame->numMappedOnThisTotal << " frames mapped on to keyframe " << _currentKeyFrame->id() << ", considering " << set->refFrame()->id() << " as new keyframe.";

				LOG_IF( DEBUG, Conf().print.threadingInfo ) << "Pushing tracked frame to mapThread.";

				// Push to mapping before checking if its the new keyframe
        _system.mapThread()->doMapSet( _currentKeyFrame, set );

				//if(!_system.mapThread->newImageSetPending() && _currentKeyFrame->frame()->numMappedOnThisTotal > MIN_NUM_MAPPED)
				if( !_newKeyFramePending && _currentKeyFrame->numMappedOnThisTotal > MIN_NUM_MAPPED)
        {
          Sophus::Vector3d dist = newRefToFrame_poseUpdate.translation() * _currentKeyFrame->frame()->meanIdepth;
          float minVal = fmin(0.2f + _system.keyFrameGraph()->size() * 0.8f / INITIALIZATION_PHASE_COUNT,1.0f);

          if(_system.keyFrameGraph()->size() < INITIALIZATION_PHASE_COUNT)	minVal *= 0.7;

          lastTrackingClosenessScore = _system.trackableKeyFrameSearch()->getRefFrameScore(dist.dot(dist), _tracker->pointUsage);

          if (lastTrackingClosenessScore > minVal)
	        {
            LOG(INFO) << "Telling mapping thread to make " << set->refFrame()->id() << " the new keyframe.";

            //CREATE THE NEW FRAME/SET
            //_system.mapThread->createNewKeyFrame( set->refFrame() );

						//TODO.  Need to add synchronization so we don't propose multiple new keyframes
						_newKeyFramePending = true;
            _system.mapThread()->doCreateNewKeyFrame( _currentKeyFrame, set->refFrame() );

            LOGF_IF( INFO, Conf().print.keyframeSelectionInfo,
                                            "SELECT KEYFRAME %d on %d! dist %.3f + usage %.3f = %.3f > 1\n",set->refFrame()->id(),set->refFrame()->trackingParent()->id(), dist.dot(dist), _tracker->pointUsage, _system.trackableKeyFrameSearch()->getRefFrameScore(dist.dot(dist), _tracker->pointUsage));
	        }
          else
          {
            LOGF_IF( INFO, Conf().print.keyframeSelectionInfo,
                                            "SKIPPD KEYFRAME %d on %d! dist %.3f + usage %.3f = %.3f > 1\n",set->refFrame()->id(),set->refFrame()->trackingParent()->id(), dist.dot(dist), _tracker->pointUsage, _system.trackableKeyFrameSearch()->getRefFrameScore(dist.dot(dist), _tracker->pointUsage));
          }
        }

        LOG_IF( DEBUG, Conf().print.threadingInfo ) << "Exiting trackFrame";

}

void TrackingThread::useNewKeyFrameImpl( const std::shared_ptr<KeyFrame> &kf )
{
	LOG(WARNING) << "Taking " << kf->id() << " as new keyframe";
	_newKeyFramePending = false;
	_currentKeyFrame = kf;
}

/*
void TrackingThread::trackFrame( const std::shared_ptr<Frame> &newFrame )
{

	if(!_trackingIsGood) {
		// Prod mapping to check the relocalizer
		_system.mapThread->relocalizer.updateCurrentFrame(newFrame);
		_system.mapThread->pushDoIteration();

//		unmappedTrackedFrames.notifyAll();

		// {
		// 	std::lock_guard< std::mutex > lock( unmappedTrackedFramesMutex );
		// 	unmappedTrackedFramesSignal.notify_one();
		// }
		return;
	}

	// Are the following two calls atomic enough or should I lock before the next two lines?
	_system.currentKeyFrame()->frameMutex.lock();
	Frame::SharedPtr keyframe( _system.currentKeyFrame() );
	_system.currentKeyFrame()->frameMutex.unlock();

	if(_trackingReference->frameID != keyframe->id() || keyframe->depthHasBeenUpdatedFlag ) {
		LOG(DEBUG) << "Importing new tracking reference from frame " << keyframe->id();
		_trackingReference->importFrame( keyframe );
		keyframe->depthHasBeenUpdatedFlag = false;
		_trackingReferenceFrameSharedPT = keyframe;
	}

	FramePoseStruct &trackingReferencePose( *_trackingReference->keyframe->pose);

	// DO TRACKING & Show tracking result.
	LOG_IF(DEBUG, Conf().print.threadingInfo) << "TRACKING frame " << newFrame->id() << " onto ref. " << _trackingReference->frameID;


	SE3 frameToReference_initialEstimate;
	{
		boost::shared_lock_guard<boost::shared_mutex> lock( _system.poseConsistencyMutex );
		frameToReference_initialEstimate = se3FromSim3( trackingReferencePose.getCamToWorld().inverse() * _system.keyFrameGraph()->allFramePoses.back()->getCamToWorld());
	}


	Timer timer;

	LOG(DEBUG) << "Start tracking...";
	SE3 newRefToFrame_poseUpdate = _tracker->trackFrame( _trackingReference,
																										newFrame,
																										frameToReference_initialEstimate);

	LOG(DEBUG) << "Done tracking, took " << timer.stop() * 1000 << " ms";
	_perf.track.update( timer );


	tracking_lastResidual = _tracker->lastResidual;
	tracking_lastUsage = _tracker->pointUsage;
	//tracking_lastGoodPerBad = _tracker->lastGoodCount / (_tracker->lastGoodCount + _tracker->lastBadCount);
	//tracking_lastGoodPerTotal = _tracker->lastGoodCount / (newFrame->width(SE3TRACKING_MIN_LEVEL)*newFrame->height(SE3TRACKING_MIN_LEVEL));


	if(manualTrackingLossIndicated || _tracker->diverged ||
		(_system.keyFrameGraph()->keyframesAll.size() > INITIALIZATION_PHASE_COUNT && !_tracker->trackingWasGood))
	{
		LOGF(WARNING, "TRACKING LOST for frame %d (%1.2f%% good Points, which is %1.2f%% of available points; %s tracking; tracker has %s)!\n",
				newFrame->id(),
				100*_tracker->_pctGoodPerTotal,
				100*_tracker->_pctGoodPerGoodBad,
				_tracker->trackingWasGood ? "GOOD" : "BAD",
				_tracker->diverged ? "DIVERGED" : "NOT DIVERGED");

		_trackingReference->invalidate();

		setTrackingIsBad();
		//nextRelocIdx = -1;  // What does this do?

		// Kick over the mapping thread
		_system.mapThread->pushDoIteration();
		// unmappedTrackedFrames.notifyAll();

		// unmappedTrackedFramesMutex.lock();
		// unmappedTrackedFramesSignal.notify_one();
		// unmappedTrackedFramesMutex.unlock();

		manualTrackingLossIndicated = false;
		return;
	}



	// if(plotTracking)
	// {
	// 	Eigen::Matrix<float, 20, 1> data;
	// 	data.setZero();
	// 	data[0] = _tracker->lastResidual;
	//
	// 	data[3] = _tracker->lastGoodCount / (tracker->lastGoodCount + _tracker->lastBadCount);
	// 	data[4] = 4*tracker->lastGoodCount / (float)_conf.slamImage.area();
	// 	data[5] = _tracker->pointUsage;
	//
	// 	data[6] = _tracker->affineEstimation_a;
	// 	data[7] = _tracker->affineEstimation_b;
	// 	outputWrapper->publishDebugInfo(data);
	// }

	_system.keyFrameGraph()->addFrame(newFrame);


	//Sim3 lastTrackedCamToWorld = mostCurrentTrackedFrame->getCamToWorld();
//  mostCurrentTrackedFrame->TrackingParent->getCamToWorld() * sim3FromSE3(mostCurrentTrackedFrame->thisToParent_SE3TrackingResult, 1.0);

	LOG_IF( DEBUG,  Conf().print.threadingInfo ) << "Publishing tracked frame";
	_system.publishTrackedFrame(newFrame);
	_system.publishPose(newFrame->getCamToWorld().cast<float>());

	// Keyframe selection
	// latestTrackedFrame = trackingNewFrame;
	//if (!my_createNewKeyframe && _map.currentKeyFrame()->numMappedOnThisTotal > MIN_NUM_MAPPED)
	LOG(INFO) << "While tracking " << newFrame->id() << " the keyframe is " << _system.currentKeyFrame()->id();
	LOG_IF( INFO, Conf().print.threadingInfo ) << _system.currentKeyFrame()->numMappedOnThisTotal << " frames mapped on to keyframe " << _system.currentKeyFrame()->id() << ", considering " << newFrame->id() << " as new keyframe.";

	if(!_system.mapThread->newKeyFramePending() && _system.currentKeyFrame()->numMappedOnThisTotal > MIN_NUM_MAPPED)
	{
		Sophus::Vector3d dist = newRefToFrame_poseUpdate.translation() * _system.currentKeyFrame()->meanIdepth;
		float minVal = fmin(0.2f + _system.keyFrameGraph()->size() * 0.8f / INITIALIZATION_PHASE_COUNT,1.0f);

		if(_system.keyFrameGraph()->size() < INITIALIZATION_PHASE_COUNT)	minVal *= 0.7;

		lastTrackingClosenessScore = _system.trackableKeyFrameSearch()->getRefFrameScore(dist.dot(dist), _tracker->pointUsage);

		if (lastTrackingClosenessScore > minVal)
		{
			LOG(INFO) << "Telling mapping thread to make " << newFrame->id() << " the new keyframe.";
			_system.mapThread->createNewKeyFrame( newFrame );
			// createNewKeyFrame = true;

			LOGF_IF( INFO, Conf().print.keyframeSelectionInfo,
							"SELECT KEYFRAME %d on %d! dist %.3f + usage %.3f = %.3f > 1\n",newFrame->id(),newFrame->trackingParent()->id(), dist.dot(dist), _tracker->pointUsage, _system.trackableKeyFrameSearch()->getRefFrameScore(dist.dot(dist), _tracker->pointUsage));
		}
		else
		{
			LOGF_IF( INFO, Conf().print.keyframeSelectionInfo,
							"SKIPPD KEYFRAME %d on %d! dist %.3f + usage %.3f = %.3f > 1\n",newFrame->id(),newFrame->trackingParent()->id(), dist.dot(dist), _tracker->pointUsage, _system.trackableKeyFrameSearch()->getRefFrameScore(dist.dot(dist), _tracker->pointUsage));

		}
	}

	LOG_IF( DEBUG, Conf().print.threadingInfo ) << "Push unmapped tracked frame.";
	_system.mapThread->pushUnmappedTrackedFrame( newFrame );

	// unmappedTrackedFrames.notifyAll();
		// unmappedTrackedFramesSignal.notify_one();
	// }

	// If blocking is requested...
	if( !Conf().runRealTime && trackingIsGood() ){
		while( _system.mapThread->unmappedTrackedFrames.size() > 0 ) {
			_system.mapThread->trackedFramesMapped.wait( );
		}
	}

	LOG_IF( DEBUG, Conf().print.threadingInfo ) << "Exiting trackFrame";

}
*/


// n.b. this function will be called from the mapping thread.  Ensure
// locking is in place.

//TODO I don't think this is ever entered?
//Need to add pushUnmappedTrackedFrame for image set
void TrackingThread::takeRelocalizeResult( const RelocalizerResult &result  )
{
        LOG(WARNING) << "Entering takeRelocalizeResult";
	// Frame* keyframe;
	// int succFrameID;
	// SE3 succFrameToKF_init;
	// std::shared_ptr<Frame> succFrame;
	//
	// relocalizer.stop();
	// relocalizer.getResult(keyframe, succFrame, succFrameID, succFrameToKF_init);
	// assert(keyframe != 0);

	//KeyFrame::SharedPtr keyframe( _currentKeyFrame );
	// _trackingReference->importFrame( keyframe );
	// _trackingReferenceFrameSharedPT = keyframe;

	_tracker->trackFrame(
			_currentKeyFrame,
			result.successfulFrame,
			result.successfulFrameToKeyframe );

	if(!_tracker->trackingWasGood || _tracker->lastGoodCount() / (_tracker->lastGoodCount()) < 1-0.75f*(1-MIN_GOODPERGOODBAD_PIXEL))
	{
		LOG_IF(DEBUG, Conf().print.relocalizationInfo) << "RELOCALIZATION FAILED BADLY! discarding result.";
		//_trackingReference->invalidate();
	}
	else
	{
		_system.keyFrameGraph()->addFrame(result.successfulFrame );

                //TODO commenting this out in the assumption I don't need it... need to revist
                //_system.mapThread->pushUnmappedTrackedFrame( result.successfulFrame );

                 //{
                 //       std::lock_guard<std::mutex> lock( currentKeyFrameMutex );
			// createNewKeyFrame = false;
			setTrackingIsGood();
                //}
	}

}

//Patch for now...
// void TrackingThread::takeRelocalizeResult( const RelocalizerResult &result, const ImageSet::SharedPtr &set )
// {
//     LOG(WARNING) << "ENTERING takeRelocalizeResult";
//
//     // Frame::SharedPtr keyframe( _system.currentKeyFrame() );
//     // _trackingReference->importFrame( keyframe );
//     // _trackingReferenceFrameSharedPT = keyframe;
//
//     _tracker->trackFrame(
//                     _currentKeyFrame->trackingReference,()
//                     result.successfulFrame,
//                     result.successfulFrameToKeyframe );
//
//     if(!_tracker->trackingWasGood || _tracker->lastGoodCount() / (_tracker->lastGoodCount()) < 1-0.75f*(1-MIN_GOODPERGOODBAD_PIXEL))
//     {
//             LOG_IF(DEBUG, Conf().print.relocalizationInfo) << "RELOCALIZATION FAILED BADLY! discarding result.";
//             _trackingReference->invalidate();
//     }
//     else
//     {
//             _system.keyFrameGraph()->addFrame(result.successfulFrame );
//
//             //_system.mapThread->pushUnmappedTrackedSet( set );
//
//             // {
//             // 	std::lock_guard<std::mutex> lock( currentKeyFrameMutex );
//                     // createNewKeyFrame = false;
//                 setTrackingIsGood();
//             //}
//     }
//
// }
