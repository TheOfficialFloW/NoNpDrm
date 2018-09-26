cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(nonpdrm)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

add_executable(nonpdrm
  main.c
)

target_link_libraries(nonpdrm
  SceIofilemgrForDriver_stub
  SceNpDrmForDriver_stub
  SceSysclibForDriver_stub
  SceThreadmgrForDriver_stub
  taihenForKernel_stub
)

vita_create_self(nonpdrm.skprx nonpdrm CONFIG exports.yml UNSAFE)

add_custom_target(copy
  COMMAND cp nonpdrm.skprx F:/tai/nonpdrm.skprx
  DEPENDS nonpdrm.skprx
)
