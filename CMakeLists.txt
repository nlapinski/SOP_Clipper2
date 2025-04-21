## Pre project envs - everyhting is setup for windows at the moment ##
set(VCPKG_LIBRARY_LINKAGE static FORCE)
set(VCPKG_CRT_LINKAGE static FORCE)
set(VCPKG_TARGET_TRIPLET x64-windows-static FORCE)
set(VCPKG_INSTALLED_DIR C:/vcpkg/installed)
set(VCPKG_MANIFEST_MODE False)
set(VCPKG_CXX_FLAGS "/wd4996 -D_CRT_SECURE_NO_WARNINGS")
set(VCPKG_C_FLAGS "/wd4996 -D_CRT_SECURE_NO_WARNINGS")
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(PKG_CONFIG_EXECUTABLE "C:/vcpkg/packages/pkgconf_x64-windows/tools/pkgconf/pkgconf.exe")

## Project ##
cmake_minimum_required( VERSION 3.26 )
project(SOP_Clipper)

#assumes you have the static VCPKG for clipper installed, and $HFS set
#HDK module
list( APPEND CMAKE_PREFIX_PATH "$ENV{HFS}/toolkit/cmake" )

# HDK cmake
find_package( Houdini REQUIRED )
find_package(glm CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

# Clipper2
find_package(PkgConfig REQUIRED)
pkg_check_modules(Clipper2 REQUIRED IMPORTED_TARGET Clipper2)

# Clipper2Z - Might set a point ID using Z in the future for correlation
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(Clipper2Z REQUIRED IMPORTED_TARGET Clipper2Z)
# target_link_libraries(main PkgConfig::Clipper2Z)

set( library_name SOP_Clipper )

 houdini_generate_proto_headers( FILES SOP_Clipper.cpp )

# Add a library and its source files.
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT") # /MT will not work.. cant remember the flags to force it for MSVC...
add_library( ${library_name} SHARED
    SOP_Clipper.cpp
    SOP_Clipper.h
    hdk_helper.h # contain all the hdk stuff, more than needed at the moment
)

# Link against the Houdini libraries, and add required include directories and
# compile definitions.
target_link_libraries( ${library_name} Houdini )

# link clipper2
target_link_libraries(${library_name} PkgConfig::Clipper2)

# Include ${CMAKE_CURRENT_BINARY_DIR} for the generated header.
target_include_directories( ${library_name} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(VCGLIB_INCLUDE_DIRS "C:/vcpkg/installed/x64-windows-static/include")
target_include_directories(${library_name} PRIVATE ${VCGLIB_INCLUDE_DIRS})
houdini_configure_target( ${library_name} )

set_target_properties(${library_name} PROPERTIES CXX_STANDARD 17)
set_target_properties(${library_name} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
set_target_properties(${library_name} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
