

#include "ImageSet.h"

namespace lsd_slam {

ImageSet::ImageSet(unsigned int id, const cv::Mat &img,
                   const libvideoio::Camera &cam)
    : _refFrame(0) {
  _frames.push_back(
      std::make_shared<Frame>(id, cam, img.size(), 0.0, img.data));
  _se3FromFirst.push_back(SE3());
  _frameId = id;
}

ImageSet::ImageSet(unsigned int id, const std::vector<cv::Mat> &imgs,
                   const std::vector<libvideoio::Camera> &cams,
                   const unsigned int ref) {

  _frames.push_back(std::make_shared<Frame>(id, cams.at(0), imgs.at(0).size(),
                                            0.0, imgs.at(0).data));
  _frames.push_back(std::make_shared<Frame>(id, cams.at(1), imgs.at(1).size(),
                                            0.0, imgs.at(1).data));
  _se3FromFirst.push_back(Sophus::SE3());
  _frameId = id;
  _refFrame = ref;
}

ImageSet::~ImageSet() {}

void ImageSet::pushbackFrame(const cv::Mat &img,
                             const libvideoio::Camera &cam) {
  _frames.push_back(
      std::make_shared<Frame>(_frameId, cam, img.size(), 0.0, img.data));
}
void ImageSet::setDisparityMap(float *_iDepth, uint8_t *_iDepthValid,
                               int _size) {

  disparity.iDepth = _iDepth;
  disparity.iDepthValid = _iDepthValid;
  disparity.iDepthSize = _size;
  float iDepthsum = 0.0;
  float *id = _iDepth;
  uint8_t *iv = _iDepthValid;
  int count = 0;
  for (int i = 0; i < _size; i++) {
    ++id;
    ++iv;
    bool valid = *iv;
    if (valid) {
      float idepth = *id;
      iDepthsum += idepth;
      count++;
    }
  }
  float _iDepthMean = iDepthsum / count;
  // std::cout << "sum: " << iDepthsum << " _iDepthMean: " << _iDepthMean
  //           << " size: " << _size << std::endl;
  disparity.iDepthMean = _iDepthMean;
}

} // namespace lsd_slam
