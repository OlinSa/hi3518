
PROJECT_SOURCE_DIR=$(SDK_PATH)
SRC_DIR=$(PROJECT_SOURCE_DIR)/src
LIVE555_PATH=$(PROJECT_SOURCE_DIR)/package/live
RTMPDUMP_PATH=$(PROJECT_SOURCE_DIR)/package/rtmpdump

LIB_NAME = libmedia.so
SRCS := $(wildcard $(SRC_DIR)/common/*.cpp $(SRC_DIR)/rtmp/*.cpp $(SRC_DIR)/rtsp/*.cpp)
OBJECTS := $(patsubst %.cpp, %.o, $(SRCS))
CFLAGS += 	-I$(SRC_DIR)/common \
			-I$(SRC_DIR)/rtmp \
			-I$(SRC_DIR)/rtsp \
			-I$(RTMPDUMP_PATH) \
			-I${LIVE555_PATH}/groupsock/include \
			-I${LIVE555_PATH}/BasicUsageEnvironment/include \
			-I${LIVE555_PATH}/liveMedia/include \
			-I${LIVE555_PATH}/mediaServer \
			-I${LIVE555_PATH}/UsageEnvironment/include

all:env_print $(LIB_NAME)

env_print:
	@echo "PROJECT_SOURCE_DIR:$(PROJECT_SOURCE_DIR)"
	@echo "SRCS:$(SRCS)"
	@echo "OBJECTS:$(OBJECTS)"

$(OBJECTS):%.o : %.cpp
	@echo $(OBJECTS)
	$(CXX) ${CFLAGS} -c  -fPIC  $< -o $@
$(LIB_NAME): $(OBJECTS)
	$(CXX) -shared -o $@ $^

clean:
	 find ./ -name "*.o" -exec rm {} \;
	 rm $(LIB_NAME)