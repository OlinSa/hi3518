
PROJECT_SOURCE_DIR=$(SDK_PATH)
SRC_DIR=$(PROJECT_SOURCE_DIR)/src
LIVE555_PATH=$(PROJECT_SOURCE_DIR)/package/live
RTMPDUMP_PATH=$(PROJECT_SOURCE_DIR)/package/rtmpdump

LIB_NAME = libmedia.so
SRCS := $(wildcard $(SRC_DIR)/common/*.cpp $(SRC_DIR)/rtmp/*.cpp $(SRC_DIR)/rtsp/*.cpp)
OBJECTS := $(patsubst %.cpp, %.o, $(SRCS))

# 采用include 包含Makefile, 防止影响主干脚本，采用独立命名
MEDIA_CFLAGS = 	-I$(SRC_DIR)/common \
			-I$(SRC_DIR)/rtmp \
			-I$(SRC_DIR)/rtsp \
			-I$(RTMPDUMP_PATH) \
			-I${LIVE555_PATH}/groupsock/include \
			-I${LIVE555_PATH}/BasicUsageEnvironment/include \
			-I${LIVE555_PATH}/liveMedia/include \
			-I${LIVE555_PATH}/mediaServer \
			-I${LIVE555_PATH}/UsageEnvironment/include


MEDIA_LIBS = -L$(LIVE555_PATH)/BasicUsageEnvironment\
		-L$(LIVE555_PATH)/groupsock \
		-L$(LIVE555_PATH)/liveMedia \
		-L$(LIVE555_PATH)/mediaServer \
		-L$(LIVE555_PATH)/UsageEnvironment \
		-lliveMedia -lBasicUsageEnvironment -lgroupsock  -lUsageEnvironment

all:env_print $(LIB_NAME)

env_print:
	@echo "PROJECT_SOURCE_DIR:$(PROJECT_SOURCE_DIR)"
	@echo "SRCS:$(SRCS)"
	@echo "OBJECTS:$(OBJECTS)"

$(OBJECTS):%.o : %.cpp
	@echo $(OBJECTS)
	$(CXX) ${MEDIA_CFLAGS} -c  -fPIC  $< -o $@
$(LIB_NAME): $(OBJECTS)
	$(CXX) -shared -o $@ $^ ${MEDIA_LIBS}

clean:
	 find ./ -name "*.o" -exec rm {} \;
	 rm $(LIB_NAME)