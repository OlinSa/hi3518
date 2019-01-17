#ifndef RING_BUFFER_HH
#define RING_BUFFER_HH

#include <iostream>

#define DEFAULT_SIZE (30 * 1024 )
#define DEFAULT_NUM (36)

enum RING_STATUS
{
	RING_STATUS_EMPTY = 0,
	RING_STATUS_FULL
};

typedef struct{
    unsigned char *buffer;
    int size;
    enum RING_STATUS status;
}ring_buffer_t;

class RingBuffer
{
public:
	static RingBuffer *GetInstance();
    RingBuffer(unsigned int n_size, unsigned int nmax);
    virtual ~RingBuffer();

    bool Init();

    bool PutRing(ring_buffer_t *ring_buffer);
    ring_buffer_t *GetRing();
private:
	ring_buffer_t *ring_buffer;
	unsigned int n_size;
	unsigned int n_max;

    unsigned int read_pos;
    unsigned int write_pos;
};


#endif

