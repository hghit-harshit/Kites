#include <gtest/gtest.h>
#include <windows.h>
#include <QLoggingCategory>

int main(int argc, char *argv[]) 
{
  #ifdef _WIN32
    // Set console output code page to UTF-8 (65001)
    SetConsoleOutputCP(65001);  
  #endif
  ::testing::InitGoogleTest(&argc, argv);
  //qInstallMessageHandler([](QtMsgType, const QMessageLogContext&, const QString&) {});
  //std::cout.setstate(std::ios_base::failbit);

  return RUN_ALL_TESTS();

}