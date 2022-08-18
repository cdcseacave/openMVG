cmake_minimum_required(VERSION 3.8)

project(siftgpu CXX)

# Find packages required by siftgpu
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
SET(SIFTGPU_COMPILE_DEFINITIONS ${SIFTGPU_DEFINES} _CRT_SECURE_NO_DEPRECATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
FIND_PACKAGE(Eigen3 NO_MODULE REQUIRED)
message(STATUS "Found Eigen3 Version: ${Eigen3_VERSION} Path: ${Eigen3_DIR}")
FIND_PACKAGE(OpenGL REQUIRED)
if(OpenGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIR})
	list(APPEND SIFTGPU_COMPILE_DEFINITIONS ${OpenGL_DEFINITIONS})
endif()
FIND_PACKAGE(GLEW REQUIRED)
if(GLEW_FOUND)
	include_directories(${GLEW_INCLUDE_DIRS})
	list(APPEND SIFTGPU_COMPILE_DEFINITIONS ${GLEW_DEFINITIONS})
endif()
if(UNIX)
	FIND_PACKAGE(X11 ${SYSTEM_PACKAGE_REQUIRED})
endif()
if(CUDA_FOUND)
	list(APPEND SIFTGPU_COMPILE_DEFINITIONS CUDA_SIFTGPU_ENABLED)
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
target_link_libraries(SIFTGPU PRIVATE ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${GLUT_LIBRARIES} ${X11_LIBRARIES} ${CUDA_LIBRARIES} Eigen3::Eigen)
set_property(TARGET SIFTGPU PROPERTY FOLDER OpenMVG/3rdParty)
if (CUDA_FOUND)
	set_property(TARGET SIFTGPU PROPERTY CUDA_ARCHITECTURES ${CUDA_ARCHFLAGS})
endif()

install(
  TARGETS SIFTGPU
  DESTINATION lib
  EXPORT openMVG-targets
)
