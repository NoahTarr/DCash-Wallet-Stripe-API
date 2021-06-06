cmake_minimum_required(VERSION 3.0.0)
project(dcash VERSION 0.1.0)

include(CTest)
enable_testing()


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)


include_directories(../include ../shared/include)


file(GLOB project_SRC CONFIGURE_DEPENDS "wallet.cpp" "../shared/*.cpp" "../shared/include/*.h")


add_executable(dcash ${project_SRC})


target_link_libraries(dcash  ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(dcash OpenSSL::SSL)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)