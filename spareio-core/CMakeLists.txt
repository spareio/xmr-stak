project(spareio-core)

cmake_minimum_required(VERSION 3.4.0)

set(CMAKE_CXX_STANDARD 17)
set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.68 REQUIRED COMPONENTS system
                                            regex
                                            program_options
                                            thread
                                            filesystem)

set(SOURCES_FILES
        NetworkManager.cpp
        NetworkManager.h
        NetworkManagerHelper.cpp
        NetworkManagerHelper.h
        TelemetryHelper.cpp
        TelemetryHelper.h
        TimeZone.cpp
        TimeZone.h
        Logger.cpp
        Logger.h
        Periodic.cpp
        Periodic.h
        RegistryHelper.cpp
        RegistryHelper.h)

include_directories(BEFORE ../)
add_library(spareio-core STATIC ${SOURCES_FILES})

find_package(OpenSSL)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    target_link_libraries(spareio-core ${OPENSSL_LIBRARIES})
    target_include_directories(spareio-core PUBLIC ${OPENSSL_INCLUDE_DIR})
else()
    message(FATAL_ERROR "OpenSSL NOT found")
endif()

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(spareio-core ${Boost_LIBRARIES})
    target_include_directories(spareio-core PUBLIC ${Boost_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Boost NOT found")
endif()