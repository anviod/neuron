# MQTT Plugin
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/plugins")

set(src 
	mqtt.c 
	util.c
	command/read_write.c
	command/group_config.c
	command/datatag.c
	command/node.c
	command/ping.c
	command/plugin.c
	command/tty.c
	command/schema.c
	command/command.c)

add_library(plugin-mqtt SHARED ${src})

target_include_directories(plugin-mqtt PRIVATE 
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/include)

target_link_libraries(plugin-mqtt neuron-base)
target_link_libraries(plugin-mqtt ${CMAKE_THREAD_LIBS_INIT})
