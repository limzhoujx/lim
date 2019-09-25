###
# executable
###
add_executable(http_demo http_demo.cpp)
target_link_libraries(http_demo lim)

add_executable(websocket_demo websocket_demo.cpp)
target_link_libraries(websocket_demo lim)

if(ENABLE_OPENSSL)
	add_executable(https_demo https_demo.cpp)
	target_link_libraries(https_demo lim)
endif()