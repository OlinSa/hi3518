
PROJECT_SOURCE_DIR=../
LIVE555_PATH=$(PROJECT_SOURCE_DIR)/package/live

LIB_NAME = libmedia.so
SRCS := $(wildcard common/*.cpp rtmp/*.cpp rtsp/*.cpp)
OBJECTS := $(patsubst %.cpp, %.o, $(SRCS))
CFLAGS := 	-Icommon \
			-Irtmp \
			-Irtsp \
			-I../package/rtmpdump \
			-I${LIVE555_PATH}/groupsock/include \
			-I${LIVE555_PATH}/BasicUsageEnvironment/include \
			-I${LIVE555_PATH}/liveMedia/include \
			-I${LIVE555_PATH}/mediaServer \
			-I${LIVE555_PATH}/UsageEnvironment/include

all:env_print $(LIB_NAME)

env_print:
	@echo "SRCS:$(SRCS)"
	@echo "OBJECTS:$(OBJECTS)"

$(OBJECTS):%.o : %.cpp
	@echo $(OBJECTS)
	$(CXX) ${CFLAGS} -c  -fPIC  $< -o $@
$(LIB_NAME): $(OBJECTS)
	$(CXX) -shared -o $@ $^

.PHONY:clean
clean:
	 find ./ -name "*.o" -exec rm {} \;
	 rm $(LIB_NAME)