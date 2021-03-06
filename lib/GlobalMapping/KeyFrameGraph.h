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
#include <vector>
#include <unordered_map>
#include <mutex>
#include <deque>

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "util/EigenCoreInclude.h"
#include <g2o/core/sparse_optimizer.h>

#include "util/SophusUtil.h"

#include "DataStructures/KeyFrame.h"

namespace lsd_slam
{


class KeyFrameGraph;
class VertexSim3;
class EdgeSim3;
class FramePoseStruct;

struct KFConstraintStruct
{
	inline KFConstraintStruct()
	{
		firstFrame = secondFrame = 0;
		information.setZero();
		robustKernel = 0;
		edge = 0;

		usage = meanResidual = meanResidualD = meanResidualP = 0;
		reciprocalConsistency = 0;


		idxInAllEdges = -1;
	}

	~KFConstraintStruct();


	KeyFrame::SharedPtr firstFrame;
	KeyFrame::SharedPtr secondFrame;
	Sophus::Sim3d secondToFirst;
	Eigen::Matrix<double, 7, 7> information;
	g2o::RobustKernel* robustKernel;
	EdgeSim3* edge;

	float usage;
	float meanResidualD;
	float meanResidualP;
	float meanResidual;

	float reciprocalConsistency;

	int idxInAllEdges;
};






/**
 * Graph consisting of KeyFrames and constraints, performing optimization.
 */
class KeyFrameGraph
{
friend class IntegrationTest;
public:
	/** Constructs an empty pose graph. */
	KeyFrameGraph();

	/** Deletes the g2o graph. */
	~KeyFrameGraph();

	/** Adds a new KeyFrame to the graph. */
	void addKeyFrame( const KeyFrame::SharedPtr &keyframe);
	void dropKeyFrame( const KeyFrame::SharedPtr &keyframe );

	/** Adds a new Frame to the graph. Doesnt actually keep the frame, but only it's pose-struct. */
//	void addFrame(const Frame::SharedPtr &frame);

	void dumpMap(std::string folder);

	/**
	 * Adds a new constraint to the graph.
	 *
	 * The transformation must map world points such that they move as if
	 * attached to a frame which moves from firstFrame to secondFrame:
	 * second->camToWorld * first->worldToCam * point
	 *
	 * If isOdometryConstraint is set, scaleInformation is ignored.
	 */
	void insertConstraint(KFConstraintStruct* constraint);

	int size() const { return keyframesAll.size(); }

	/** Optimizes the graph. Does not update the keyframe poses,
	 *  only the vertex poses. You must call updateKeyFramePoses() afterwards. */
	int optimize(int num_iterations);
	bool addElementsFromBuffer();


	/**
	 * Creates a hash map of keyframe -> distance to given frame.
	 */
	void calculateGraphDistancesToFrame(const KeyFrame::SharedPtr &frame, std::unordered_map<KeyFrame::SharedPtr, int> &distanceMap);


	int totalPoints;
	int totalEdges;
	int totalVertices;


	//=========================== Keyframe & Pose Lists & Maps ====================================
	// Always lock the list with the corresponding mutex!
	// central point to administer keyframes, iterate over keyframes, do lookups etc.

	std::vector< KeyFrame::SharedPtr > _keyFrames;

	// contains ALL keyframes, as soon as they are "finished".
	// does NOT yet contain the keyframe that is currently being created.
	boost::shared_mutex keyframesAllMutex;
	std::vector< KeyFrame::SharedPtr > keyframesAll;


	/** Maps frame ids to keyframes. Contains ALL Keyframes allocated, including the one that currently being created. */
	/* this is where the shared pointers of Keyframe Frames are kept, so they are not deleted ever */
	boost::shared_mutex idToKeyFrameMutex;
	std::unordered_map< int, KeyFrame::SharedPtr > idToKeyFrame;


	// contains ALL edges, as soon as they are created
	boost::shared_mutex edgesListsMutex;
	std::vector< KFConstraintStruct* > edgesAll;



	// contains ALL frame poses, chronologically, as soon as they are tracked.
	// the corresponding frame may have been removed / deleted in the meantime.
	// these are the ones that are also referenced by the corresponding Frame / Keyframe object
	// boost::shared_mutex allFramePosesMutex;
	// std::vector< FramePoseStruct::SharedPtr  > allFramePoses;


	// contains all keyframes in graph, in some arbitrary (random) order. if a frame is re-tracked,
	// it is put to the end of this list; frames for re-tracking are always chosen from the first third of
	// this list.
	std::mutex keyframesForRetrackMutex;
	std::deque< KeyFrame::SharedPtr > keyframesForRetrack;

	// Used to live with the optimizer, but not needed as signal anymore,
	// now just used as a simple data mutex.
	std::mutex newKeyFrameMutex;


private:

	/** Pose graph representation in g2o */
	g2o::SparseOptimizer graph;

	std::vector< KeyFrame::SharedPtr > newKeyframesBuffer;
	std::vector< KFConstraintStruct* > newEdgeBuffer;


	int nextEdgeId;
};

}
