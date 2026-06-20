# A Zap sample project.
#
# To build (non-MSVC):
#   1. Install Zap somewhere ($PREFIX below):
#
#      mkdir zap/build
#      cd zap/build
#      cmake ../c++ -DCMAKE_INSTALL_PREFIX=$PREFIX
#      cmake --build . --target install
#
#   2. Ensure Zap's executables are on the PATH, then build the sample project:
#
#      export PATH=$PREFIX/bin:$PATH
#      mkdir ../build-samples
#      cd ../build-samples
#      cmake ../c++/samples
#      cmake --build .

cmake_minimum_required(VERSION 3.16)
project("Zap Samples" CXX)

find_package(Zap CONFIG REQUIRED)

zap_generate_cpp(addressbookSources addressbookHeaders addressbook.zap)
add_executable(addressbook addressbook.c++ ${addressbookSources})
target_link_libraries(addressbook PRIVATE Zap::zap)
target_include_directories(addressbook PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Don't build the rpc sample if find_package() found an installation of Zap lite.
if(TARGET Zap::zap-rpc)
  zap_generate_cpp(calculatorSources calculatorHeaders calculator.zap)
  add_executable(calculator-client calculator-client.c++ ${calculatorSources})
  add_executable(calculator-server calculator-server.c++ ${calculatorSources})
  target_link_libraries(calculator-client PRIVATE Zap::zap-rpc)
  target_link_libraries(calculator-server PRIVATE Zap::zap-rpc)
  target_include_directories(calculator-client PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_include_directories(calculator-server PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()
