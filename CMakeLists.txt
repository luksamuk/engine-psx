cmake_minimum_required(VERSION 3.21)

project(PSXEngine
  LANGUAGES    C CXX ASM
  VERSION      1.0.0
  DESCRIPTION  "Platformer Engine for PSX"
  HOMEPAGE_URL "https://luksamuk.codes")

file(GLOB ENGINE_SRC
  ${CMAKE_CURRENT_LIST_DIR}/src/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/*.s)

set(CMAKE_C_FLAGS "-Wall")

psn00bsdk_add_executable(engine
  GPREL
  ${ENGINE_SRC})

target_include_directories(engine PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/>)

psn00bsdk_add_cd_image(iso engine
  iso.xml
  DEPENDS engine system.cnf)

