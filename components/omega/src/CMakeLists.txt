# build Omega

# Add source files for the library
file(GLOB _LIBSRC_FILES infra/*.cpp base/*.cpp ocn/*.cpp)

# Create the library target
add_library(${OMEGA_LIB_NAME} ${_LIBSRC_FILES})

# add include directories
target_include_directories(
    ${OMEGA_LIB_NAME}
    PRIVATE
    ${OMEGA_SOURCE_DIR}/src/base
    ${OMEGA_SOURCE_DIR}/src/infra
    ${Parmetis_INCLUDE_DIRS}
)

# add compiler options
target_include_directories(
    ${OMEGA_LIB_NAME}
    PRIVATE
    ${OMEGA_SOURCE_DIR}/src/base
    ${OMEGA_SOURCE_DIR}/src/infra
    ${OMEGA_SOURCE_DIR}/src/ocn
)

target_compile_options(
    ${OMEGA_LIB_NAME}
    PRIVATE
     ${OMEGA_CXX_FLAGS}
)

# add linker options
target_link_options(
    ${OMEGA_LIB_NAME}
    PRIVATE
    ${OMEGA_LINK_OPTIONS}
)

target_link_libraries(${OMEGA_LIB_NAME} spdlog yakl pioc yaml-cpp parmetis metis)

if(GKlib_FOUND)
    target_link_libraries(${OMEGA_LIB_NAME} gklib)
endif()

# build Omega executable
if(OMEGA_BUILD_EXECUTABLE)

  set(EXESRC_FILES
    drivers/DrvDummy.cpp
  )

  # Create the executable target
  add_executable(${OMEGA_EXE_NAME} ${EXESRC_FILES})

  target_compile_options(
		${OMEGA_EXE_NAME}
		PRIVATE
		"-L${OMEGA_SOURCE_DIR}/src/base"
		"-L${CMAKE_CURRENT_SOURCE_DIR}/infra"
  )

  target_link_libraries(${OMEGA_EXE_NAME} ${OMEGA_LIB_NAME})

endif()
