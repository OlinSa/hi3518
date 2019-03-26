set(LIVE555_PATH  ${PROJECT_SOURCE_DIR}/package/live)

include_directories(${PROJECT_SOURCE_DIR}/src/common)
include_directories(${LIVE555_PATH}/groupsock/include)
include_directories(${LIVE555_PATH}/BasicUsageEnvironment/include)
include_directories(${LIVE555_PATH}/liveMedia/include)
include_directories(${LIVE555_PATH}/mediaServer)
include_directories(${LIVE555_PATH}/UsageEnvironment/include)

AUX_SOURCE_DIRECTORY(. DIR_RTSP)
add_executable(live ${DIR_RTSP})

link_directories(${LIVE555_PATH}/BasicUsageEnvironment)
link_directories(${LIVE555_PATH}/groupsock)
link_directories(${LIVE555_PATH}/liveMedia)
link_directories(${LIVE555_PATH}/UsageEnvironment)

target_link_libraries(live common)
target_link_libraries(live liveMedia)
target_link_libraries(live BasicUsageEnvironment)
target_link_libraries(live groupsock)
target_link_libraries(live UsageEnvironment)