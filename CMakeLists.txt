###
# config
###
cmake_minimum_required(VERSION 2.8)

###
# project
###
set(PROJECT lim)
project(${PROJECT} CXX)
ENABLE_TESTING()

OPTION(ENABLE_OPENSSL "Define if build without support for OpenSSL encryption" OFF)

###
# compilation options
###
#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)
if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -DEBUG -g -O0")
endif()


###
# configure file setting
###
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/include/lim/config.h")
  
###
# includes
###
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
###
# sources
###
set(HEADER_FILES
	include/lim/base/base64.h
    include/lim/base/bootstrap.h
    include/lim/base/bootstrap_config.h
    include/lim/base/byte_buffer.h
	include/lim/base/connected_channel_session.h
	include/lim/base/event_loop.h
	include/lim/base/execute_task.h
	include/lim/base/logger.h
	include/lim/base/message_decoder.h
	include/lim/base/server_channel_session.h
	include/lim/base/sha1.h
	include/lim/base/socket_channel.h
	include/lim/base/string_utils.h
    include/lim/base/time_utils.h
	include/lim/http/http_message.h
	include/lim/http/http_bootstrap_config.h
	include/lim/http/http_base_decoder.h
	include/lim/http/http_request_session.h
	include/lim/http/http_response_session.h
	include/lim/websocket/websocket_frame_message.h
	include/lim/websocket/websocket_bootstrap_config.h
	include/lim/websocket/websocket_frame_decoder.h
	include/lim/websocket/websocket_request_session.h
	include/lim/websocket/websocket_response_session.h)
	
set(SOURCE_FILES
	src/base/base64.cpp
    src/base/byte_buffer.cpp
	src/base/connected_channel_session.cpp
	src/base/event_loop.cpp
	src/base/execute_task.cpp
	src/base/logger.cpp
	src/base/sha1.cpp
	src/base/socket_channel.cpp
	src/base/string_utils.cpp
    src/base/time_utils.cpp
	src/http/http_message.cpp
	src/http/http_base_decoder.cpp
	src/http/http_request_session.cpp
	src/http/http_response_session.cpp
	src/websocket/websocket_frame_message.cpp
	src/websocket/websocket_frame_decoder.cpp
	src/websocket/websocket_request_session.cpp
	src/websocket/websocket_response_session.cpp)

if(ENABLE_OPENSSL)
	find_package(OpenSSL REQUIRED)
    
	message(STATUS "OpenSSL include: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OpenSSL lib: ${OPENSSL_LIBRARIES}")
	
    include_directories(${OPENSSL_INCLUDE_DIR})
	list(APPEND HEADER_FILES include/lim/base/sslcontext.h)
	list(APPEND SOURCE_FILES src/base/sslcontext.cpp)
    list(APPEND OPTION_LIBS ${OPENSSL_LIBRARIES})
	
	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/lim DESTINATION include)
endif()

###
# outputs
###
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

###
# executable
###
add_library(${PROJECT} ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(${PROJECT} ${OPTION_LIBS} pthread)

###
# examples
###
add_subdirectory(examples)


###
# install
###
install(TARGETS ${PROJECT} ${PROJECT}  
       #RUNTIME DESTINATION bin  
       LIBRARY DESTINATION lib  
       ARCHIVE DESTINATION lib  
)  

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/lim DESTINATION include)
