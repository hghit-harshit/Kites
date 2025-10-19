include("D:/RISC_V_Simulator/build/Desktop_Qt_6_9_2_MinGW_64_bit-Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/RISC_V_Simulator-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "D:/RISC_V_Simulator/build/Desktop_Qt_6_9_2_MinGW_64_bit-Debug/RISC_V_Simulator.exe"
    GENERATE_QT_CONF
)
