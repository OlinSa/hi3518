# Hisilicon Hi3516 sample Makefile

include ../Makefile.param
#ifeq ($(SAMPLE_PARAM_FILE), )
#     SAMPLE_PARAM_FILE:=../Makefile.param
#     include $(SAMPLE_PARAM_FILE)
#endif
include ../../../src/Makefile

# target source
ROOT := /home/cuijian/do/git/hi3518/
LIVE555_PATH := /home/cuijian/do/git/hi3518/package/live
SRC  := $(wildcard *.cpp) 

OBJ  := $(SRC:%.cpp=%.o)
INC  := -I../common	\
		-I../../include \
		-I$(LIVE555_PATH)/BasicUsageEnvironment/include \
		-I$(LIVE555_PATH)/groupsock/include \
		-I$(LIVE555_PATH)/liveMedia/include \
		-I$(LIVE555_PATH)/mediaServer/include \
		-I$(LIVE555_PATH)/UsageEnvironment/include


# objects := $(patsubst %.c,%.o,$(wildcard *.c))

CFLAGS += $(INC)
CXXLIBS = -L$(LIVE555_PATH)/BasicUsageEnvironment\
		-L$(LIVE555_PATH)/groupsock \
		-L$(LIVE555_PATH)/liveMedia \
		-L$(LIVE555_PATH)/mediaServer \
		-L$(LIVE555_PATH)/UsageEnvironment \
		-lliveMedia -lBasicUsageEnvironment -lgroupsock  -lUsageEnvironment  \
		-L. -lmedia -L$(RTMPDUMP_PATH)/librtmp -lrtmp

TARGET := video_sample
# TARGET := $(OBJ:%.o=%)
.PHONY : clean all

$(info $(OBJ))

all: $(TARGET)

$(TARGET):$(OBJ) $(COMM_OBJ)
	$(CXX) $(CFLAGS) -lpthread -lm -o $@ $^ $(MPI_LIBS) $(AUDIO_LIBA) $(SENSOR_LIBS) $(CXXLIBS)
%.o:%.cpp
	$(CXX) $(CFLAGS)  -c -o $@ $^

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ)
	@rm -f $(COMM_OBJ)

cleanstream:
	@rm -f *.h264
	@rm -f *.jpg
	@rm -f *.mjp
	@rm -f *.mp4
