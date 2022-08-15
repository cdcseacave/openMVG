#ifndef OPENMVG_FEATURES_SIFT_SIFTGPU_IMAGE_DESCRIBER_HPP
#define OPENMVG_FEATURES_SIFT_SIFTGPU_IMAGE_DESCRIBER_HPP

#include <numeric>
#include <vector>

#include "openMVG/features/feature.hpp"
#include "openMVG/features/image_describer.hpp"
#include "openMVG/features/regions_factory.hpp"
#include "openMVG/features/sift/hierarchical_gaussian_scale_space.hpp"
#include "openMVG/features/sift/sift_DescriptorExtractor.hpp"
#include "openMVG/features/sift/sift_keypoint.hpp"
#include "openMVG/features/sift/sift_KeypointExtractor.hpp"
#include "openMVG/system/logger.hpp"


class SiftGPU;

namespace openMVG {
namespace features {
	
class SIFTGPU_Image_describer : public Image_describer
{
public:

  using Regions_type = SIFT_Regions;

  struct Params
  {
    Params(
      int num_features = 0,
      int first_octave = 0,
      int num_octaves = 6,
      int num_scales = 3,
      float edge_threshold = 10.0f,
      float peak_threshold = 0.04f,
      bool root_sift = true
    ):
      num_features_(num_features),
      first_octave_(first_octave),
      num_octaves_(num_octaves),
      num_scales_(num_scales),
      edge_threshold_(edge_threshold),
      peak_threshold_(peak_threshold),
      root_sift_(root_sift) {}

    template<class Archive>
    inline void serialize( Archive & ar );

    // Parameters
    int num_features_;      // Maximum number of features per image, 0 - no limit
    int first_octave_;      // Use original image, or perform an upscale if == -1
    int num_octaves_;       // Max octaves count
    int num_scales_;        // Scales per octave
    float edge_threshold_;  // Max ratio of Hessian eigenvalues
    float peak_threshold_;  // Min contrast
    bool root_sift_;        // see [1]
  };

  explicit SIFTGPU_Image_describer
  (
    const Params & params = Params()
  ):
    Image_describer(),
    params_(params)
  {}


  bool Set_configuration_preset(EDESCRIBER_PRESET preset) override
  {
    switch (preset)
    {
    case NORMAL_PRESET:
      params_.peak_threshold_ = 0.005f;
    break;
    case HIGH_PRESET:
      params_.peak_threshold_ = 0.001f;
    break;
    case ULTRA_PRESET:
      params_.peak_threshold_ = 0.001f;
      params_.first_octave_ = -1;
    break;
    default:
      OPENMVG_LOG_ERROR << "Invalid preset configuration";
      return false;
    }
    return true;
  }

  void SetFeaturesMax(unsigned);
  void SetImageSizeMax(unsigned);
  void SetDevice(int nDevice=-1);
  void SetFactor(float fFilterWidth=4.f, float fOrientationSampleWindow=2.f, float fDescriptorGridSize=3.f);
  void SetDOG(int nLevelsPerOctave=3, float fThreshold=0.02f/3, float fEdgeThreshold=10.f);
  void SetFirstOctave(int firstOctave=-1);
  void SetDarknessAdaptation(bool bEnable=true);
  bool IsExecutedOnCPU() const;

  bool Init();

  /**
  @brief Detect regions on the image and compute their attributes (description)
  @param image Image.
  @param mask 8-bit gray image for keypoint filtering (optional).
     Non-zero values depict the region of interest.
  @return regions The detected regions and attributes (the caller must delete the allocated data)
  */
  std::unique_ptr<Regions_type> Describe_SIFTGPU(
    const image::Image<unsigned char>& image,
    const image::Image<unsigned char>* mask = nullptr
  );

  std::unique_ptr<Regions> Allocate() const override
  {
    return std::unique_ptr<Regions_type>(new Regions_type);
  }

  template<class Archive>
  inline void serialize( Archive & ar );

  std::unique_ptr<Regions> Describe(
    const image::Image<unsigned char>& image,
    const image::Image<unsigned char>* mask = nullptr
  ) override
  {
    return Describe_SIFTGPU(image, mask);
  }

 private:
  Params params_;
  std::shared_ptr<SiftGPU> pSiftGPU_;
};

} // namespace features
} // namespace openMVG


#endif // OPENMVG_FEATURES_SIFT_SIFTGPU_IMAGE_DESCRIBER_HPP
