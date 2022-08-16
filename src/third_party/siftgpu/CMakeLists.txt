cmake_minimum_required(VERSION 3.8)

project(siftgpu CXX)

# Find packages required by siftgpu
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
SET(SIFTGPU_COMPILE_DEFINITIONS ${SIFTGPU_DEFINES} _CRT_SECURE_NO_DEPRECATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
FIND_PACKAGE(OpenGL ${SYSTEM_PACKAGE_REQUIRED})
if(OpenGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIR})
	list(APPEND SIFTGPU_COMPILE_DEFINITIONS ${OpenGL_DEFINITIONS})
endif()
FIND_PACKAGE(GLEW ${SYSTEM_PACKAGE_REQUIRED})
if(GLEW_FOUND)
	include_directories(${GLEW_INCLUDE_DIRS})
	list(APPEND SIFTGPU_COMPILE_DEFINITIONS ${GLEW_DEFINITIONS})
endif()
SET(CUDA_FOUND FALSE)
SET(CUDA_LIBRARIES "")
if(USE_CUDA)
	FIND_PACKAGE(CUDA 7.0 QUIET)
	if(CUDA_FOUND)
        include(${CMAKE_SOURCE_DIR}/cmakeFindModules/SelectCudaComputeArch.cmake)
		set(CUDA_ARCHS "Auto")
        CUDA_SELECT_NVCC_ARCH_FLAGS(CUDA_ARCH_FLAGS ${CUDA_ARCHS})
        set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} ${CUDA_ARCH_FLAGS}")
        # Fix for some combinations of CUDA and GCC (e.g. under Ubuntu 16.04).
        set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -D_FORCE_INLINES")
        # Do not show warnings if the architectures are deprecated.
        set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -Wno-deprecated-gpu-targets")
		INCLUDE_DIRECTORIES(${CUDA_INCLUDE_DIRS})
		list(APPEND SIFTGPU_COMPILE_DEFINITIONS CUDA_SIFTGPU_ENABLED)
		list(APPEND CUDA_LIBRARIES ${CUDA_CUDA_LIBRARY} ${CUDA_CUDART_LIBRARY})
		enable_language(CUDA)
        message(STATUS "Enabling CUDA support (version: ${CUDA_VERSION_STRING},"
                       " archs: ${CUDA_ARCH_FLAGS_readable})")
	else()
		MESSAGE("-- Can't find CUDA. Continuing without it.")
	endif()
endif()
if(UNIX)
	FIND_PACKAGE(X11 ${SYSTEM_PACKAGE_REQUIRED})
endif()

# list SIFTGPU sources files
set(SIFTGPU_SRCS
	GlobalUtil.cpp
	GLTexImage.cpp
	FeaturePoints.cpp
	FrameBufferObject.cpp
	MatchFile.cpp
	ProgramGLSL.cpp
	ProgramGPU.cpp
	PyramidGL.cpp
	ShaderMan.cpp
	SiftGPU.cpp
	SiftMatch.cpp
	SiftPyramid.cpp
)
if (CUDA_FOUND)
	list(APPEND SIFTGPU_SRCS
		CuTexImage.cpp
		ProgramCU.cu
		PyramidCU.cpp
		SiftMatchCU.cpp
	)
endif()

add_library(SIFTGPU STATIC ${SIFTGPU_SRCS})
set_target_properties(SIFTGPU PROPERTIES COMPILE_DEFINITIONS "${SIFTGPU_COMPILE_DEFINITIONS}")
target_link_libraries(SIFTGPU PRIVATE ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${GLUT_LIBRARIES} ${X11_LIBRARIES} ${CUDA_LIBRARIES})
set_property(TARGET SIFTGPU PROPERTY FOLDER OpenMVG/3rdParty)
if (CUDA_FOUND)
	set_property(TARGET SIFTGPU PROPERTY CUDA_ARCHITECTURES ${CUDA_ARCH_FLAGS_list})
endif()

install(
  TARGETS SIFTGPU
  DESTINATION lib
  EXPORT openMVG-targets
)
