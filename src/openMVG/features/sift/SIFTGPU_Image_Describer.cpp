#include "openMVG/features/sift/SIFTGPU_Image_Describer.hpp"
#ifdef _MSC_VER
#include <Windows.h>
#endif
#include "third_party/siftgpu/SiftGPU.h"

#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_LUMINANCE 0x1909
#define GL_BGR 0x80E0
#define GL_BGRA 0x80E1

namespace openMVG {
namespace features {

void GetDeviceIdentification(int nDevice, std::string& id) {
	#ifdef _MSC_VER
	// locate the desired device or the primary display device
	DISPLAY_DEVICE dd;
	memset(&dd, 0, sizeof(DISPLAY_DEVICE));
	dd.cb = sizeof(DISPLAY_DEVICE);
	if (nDevice < 0 || !EnumDisplayDevices(NULL, nDevice, &dd, 0)) {
		// choose the best device
		for (int i = 0; EnumDisplayDevices(NULL, i, &dd, 0); ++i)
			if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
				break;
	}
	// desired device found, select it
	id = dd.DeviceName;
	assert(!id.empty());
	#endif
}

void SIFTGPU_Image_describer::SetDevice(int nDevice /*=-1*/) {
	// choose desired device
	std::string strDevice;
	GetDeviceIdentification(nDevice, strDevice);
	if (strDevice.empty())
		return;
	const char* args[] = {
		#ifdef _USE_CUDA
		"-cuda", // enable cuda
		#endif
		#if 0
		"-loweo", "1", // set (0,0) to be the center of the pixel
		#endif
		"-display",
		strDevice.c_str()
	};
	pSiftGPU_->ParseParam((int)(sizeof(args) / sizeof(args[0])), args);
}

void SIFTGPU_Image_describer::SetFactor(float fFilterWidth /*=4.f*/,
	float fOrientationSampleWindow /*=2.f*/,
	float fDescriptorGridSize /*=3.f*/) {
	char buffer[3][64];
	sprintf(buffer[0], "%f", fFilterWidth);
	sprintf(buffer[1], "%f", fOrientationSampleWindow);
	sprintf(buffer[2], "%f", fDescriptorGridSize);
	const char* args[] = { "-f", buffer[0], "-w", buffer[1], "-dw", buffer[2] };
	pSiftGPU_->ParseParam(6, args);
}

void SIFTGPU_Image_describer::SetDOG(int nLevelsPerOctave /*=3*/,
	float fThreshold /*=0.02f/3*/,
	float fEdgeThreshold /*=10.f*/) {
	char buffer[3][64];
	sprintf(buffer[0], "%d", nLevelsPerOctave);
	sprintf(buffer[1], "%f", fThreshold);
	sprintf(buffer[2], "%f", fEdgeThreshold);
	const char* args[] = { "-d", buffer[0], "-t", buffer[1], "-e", buffer[2] };
	pSiftGPU_->ParseParam(6, args);
}

void SIFTGPU_Image_describer::SetFirstOctave(int firstOctave /*=-1*/) {
	char buffer[64];
	sprintf(buffer, "%d", firstOctave);
	const char* args[] = { "-fo", buffer };
	pSiftGPU_->ParseParam(2, args);
}

void SIFTGPU_Image_describer::SetDarknessAdaptation(bool bEnable /*=true*/) {
	if (bEnable) {
		const char* args[] = { "-da" };
		pSiftGPU_->ParseParam(1, args);
	}
}

void SIFTGPU_Image_describer::SetFeaturesMax(unsigned maxFeatures) {
	if (pSiftGPU_ == NULL)
		return;
	if (maxFeatures == 0) {
		const char* args[] = { "-tc1", "-1" };
		pSiftGPU_->ParseParam(2, args);
	}
	else {
		char buffer[64];
		sprintf(buffer, "%u", maxFeatures);
		const char* args[] = { "-tc2", buffer };
		pSiftGPU_->ParseParam(2, args);
	}
}

void SIFTGPU_Image_describer::SetImageSizeMax(unsigned size) {
	if (pSiftGPU_ == NULL)
		return;
	if (size == 0)
		size = 3200;
	pSiftGPU_->SetMaxDimension(size);
	pSiftGPU_->AllocatePyramid(size, size);
}

// check OpenGL is not using a known software renderer driver
bool SIFTGPU_Image_describer::IsExecutedOnCPU() const {
	assert(pSiftGPU_ != NULL);
	const char* szRenderer = pSiftGPU_->GetRendererInfo();
	if (szRenderer != NULL &&
		std::string(szRenderer).find("llvm") == std::string::npos)
		return false;
	return true;
}


bool SIFTGPU_Image_describer::Init() {
	pSiftGPU_ = std::make_shared<SiftGPU>();
	#ifndef _DEBUG
	pSiftGPU_->SetVerbose(-3);
	#else
	pSiftGPU_->SetVerbose(2);
	#endif
	SetDevice(-1);
	SetFactor(3.6f, 2.6f, 3.8f);
	SetDOG(4, params_.peak_threshold_, params_.edge_threshold_);
	SetFirstOctave(params_.first_octave_);
	SetDarknessAdaptation(true);
	SetFeaturesMax(params_.num_features_);
	const int ret = pSiftGPU_->CreateContextGL();
	assert(ret == SiftGPU::SIFTGPU_FULL_SUPPORTED);
	return true;
}


std::unique_ptr<SIFTGPU_Image_describer::Regions_type> SIFTGPU_Image_describer::Describe_SIFTGPU(
	const image::Image<unsigned char>& image,
	const image::Image<unsigned char>* mask
)
{
	auto regions = std::unique_ptr<Regions_type>(new Regions_type);

	if (image.size() == 0)
		return regions;

	if (!pSiftGPU_ && !Init())
		return regions;

	// compute sift keypoints
	{
		unsigned type = GL_BGR;
		switch (image.Depth()) {
		case 1: type = GL_LUMINANCE; break;
		case 3: type = GL_BGR; break;
		case 4: type = GL_BGRA; break;
		}

		// repeat till we get enough features
		const unsigned numMinFeatures = 1000;
		unsigned numFeatures = 0;
		unsigned iteration = 0, maxIterations = 3;
		do {
			// update DoG threshold
			const float old_threshold = pSiftGPU_->_dog_threshold;
			const float dogThresholdScale = std::pow(0.1f, iteration);
			pSiftGPU_->_dog_threshold *= dogThresholdScale;

			// find features
			const int ret = pSiftGPU_->RunSIFT(image.Width(), image.Height(), image.GetMat().data(), type, GL_UNSIGNED_BYTE);
			pSiftGPU_->_dog_threshold = old_threshold;
			if (!ret)
				return regions;
			numFeatures = pSiftGPU_->GetFeatureNum();
		} while (numFeatures < numMinFeatures && ++iteration < maxIterations);

		std::vector<SiftKeypoint> gpu_keypoints;
		std::vector<sift::Keypoint> keypoints;
		gpu_keypoints.resize(numFeatures);
		keypoints.reserve(numFeatures);
		regions->Descriptors().resize(numFeatures);
		pSiftGPU_->GetFeatureVector((SiftKeypoint*)gpu_keypoints.data(), (uint8_t*)regions->Descriptors().data());
		for (const SiftKeypoint& k : gpu_keypoints)
		{
			// Feature masking
			if (mask)
			{
				const image::Image<unsigned char> & maskIma = *mask;
				if (maskIma(k.y, k.x) == 0) {
					regions->Descriptors().erase(regions->Descriptors().begin() + std::distance<const SiftKeypoint*>(&gpu_keypoints.front(), &k));
					continue;
				}
			}
			// Create the SIFT region
			{
				regions->Features().emplace_back(k.x, k.y, k.s, k.o);
			}
		}
	}
	return regions;
}

} // namespace features
} // namespace openMVG
