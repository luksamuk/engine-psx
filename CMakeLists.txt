cmake_minimum_required(VERSION 3.21)

project(PSXEngine
  LANGUAGES    C ASM
  VERSION      0.1.0
  DESCRIPTION  "Sonic The Hedgehog XA for PlayStation"
  HOMEPAGE_URL "https://luksamuk.codes")

file(GLOB SONICXA_SRC
  ${CMAKE_CURRENT_LIST_DIR}/src/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/*.s)

list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/cmake/")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_get_exact_tag(GIT_TAG)

string(SUBSTRING "${GIT_SHA1}"     0  7 GIT_SHORTHASH)
string(SUBSTRING "${GIT_REFSPEC}" 11 -1 GIT_BRANCH)

add_definitions("-DGIT_SHA1=\"${GIT_SHA1}\"")
add_definitions("-DGIT_REFSPEC=\"${GIT_REFSPEC}\"")
add_definitions("-DGIT_COMMIT=\"${GIT_BRANCH}/${GIT_SHORTHASH}\"")

if(GIT_TAG)
  add_definitions("-DGIT_VERSION=\"${GIT_TAG}\"")
else()
  add_definitions("-DGIT_VERSION=\"DEV BUILD ${GIT_SHORTHASH}\"")
endif()

# Allows debug mode. Debug can be activated through the options menu at will,
add_definitions("-DALLOW_DEBUG")

# Optional test warp: configure with -DTEST_WARP_LEVEL=<id> to boot straight
# into a level; -DTEST_WARP_CHARACTER=<id> selects the character and
# -DTEST_WARP_DEBUG_MODE=1|2 activates the debug HUD/sensors.
if(TEST_WARP_LEVEL)
  add_definitions("-DTEST_WARP_LEVEL=${TEST_WARP_LEVEL}")
endif()
if(TEST_WARP_CHARACTER)
  add_definitions("-DTEST_WARP_CHARACTER=${TEST_WARP_CHARACTER}")
endif()
if(TEST_WARP_DEBUG_MODE)
  add_definitions("-DTEST_WARP_DEBUG_MODE=${TEST_WARP_DEBUG_MODE}")
endif()

set(CMAKE_C_FLAGS "-Wall")

psn00bsdk_add_executable(sonic
  GPREL
  ${SONICXA_SRC})

target_include_directories(sonic PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/>)

psn00bsdk_add_cd_image(iso "SONICXA"
  iso.xml
  DEPENDS sonic system.cnf)

