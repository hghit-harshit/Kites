# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\RISC_V_Simulator_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\RISC_V_Simulator_autogen.dir\\ParseCache.txt"
  "RISC_V_Simulator_autogen"
  )
endif()
