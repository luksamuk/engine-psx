cmake_minimum_required(VERSION 3.21)

project(SonicEngine
  LANGUAGES    C CXX ASM
  VERSION      1.0.0
  DESCRIPTION  "Sonic Engine for PSX"
  HOMEPAGE_URL "https://luksamuk.codes")

file(GLOB ENGINE_SRC ${CMAKE_CURRENT_LIST_DIR}/src/*.c)

psn00bsdk_add_executable(sonic
  GPREL
  ${ENGINE_SRC})

target_include_directories(sonic PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/>)

psn00bsdk_add_cd_image(iso sonicengine
  iso.xml
  DEPENDS sonic system.cnf)

