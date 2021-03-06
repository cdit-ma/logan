set(AGGREGATION_BROKER "aggregation_broker")
set(PROJ_NAME ${AGGREGATION_BROKER})

project(${PROJ_NAME})

#Find packages
find_package(Protobuf REQUIRED)
find_package(ZMQ REQUIRED)
find_package(Boost 1.30.0 COMPONENTS program_options REQUIRED)

set(SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../databaseclient.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../utils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/aggregationbroker.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/aggregationreplier.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

set(HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../databaseclient.h
	${CMAKE_CURRENT_SOURCE_DIR}/../utils.h
	${CMAKE_CURRENT_SOURCE_DIR}/aggregationbroker.h
	${CMAKE_CURRENT_SOURCE_DIR}/aggregationreplier.h
)

# Construct an aggregation_server binary
add_executable(${PROJ_NAME} ${SOURCES} ${HEADERS})

if (MSVC)
    # Windows requires protobuf in DLLs
	add_definitions(-DPROTOBUF_USE_DLLS)
	# Visual studio needs to be told to build in Multithreaded Dynamically Linked mode
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
else()
	# Unix needs to include pthread
	target_link_libraries(${PROJ_NAME} pthread)
    target_link_libraries(${PROJ_NAME} dl)
endif()


target_include_directories(${PROJ_NAME} PRIVATE ${PROTOBUF_INCLUDE_DIRS})
target_include_directories(${PROJ_NAME} PRIVATE ${ZMQ_INCLUDE_DIRS})
target_include_directories(${PROJ_NAME} PRIVATE ${LOGAN_SRC_PATH})
#target_include_directories(${PROJ_NAME} PRIVATE "C:/git/libpqxx/include")
#target_include_directories(${PROJ_NAME} PRIVATE "/usr/include/pqxx")

target_include_directories(${PROJ_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(${PROJ_NAME} PRIVATE ${RE_COMMON_BINARY_DIR})
target_include_directories(${PROJ_NAME} PRIVATE ${RE_COMMON_SOURCE_DIR})

target_link_libraries(${PROJ_NAME} zmq_protoreplier)
#target_link_libraries(${PROJ_NAME} zmq_protoreceiver)
#target_link_libraries(${PROJ_NAME} zmq_protowriter)
target_link_libraries(${PROJ_NAME} re_common_proto_aggregation_message)
#target_link_libraries(${PROJ_NAME} re_common_proto_modelevent)
#target_link_libraries(${PROJ_NAME} re_common_proto_control)
#target_link_libraries(${PROJ_NAME} re_common_proto_systemstatus)
target_link_libraries(${PROJ_NAME} pqxx)
target_link_libraries(${PROJ_NAME} ${Boost_PROGRAM_OPTIONS_LIBRARY})
target_link_libraries(${PROJ_NAME} ${PROTOBUF_LIBRARIES})