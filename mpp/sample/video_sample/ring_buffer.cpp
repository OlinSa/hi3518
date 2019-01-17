#include <string.h>
#include "ring_buffer.hh"

using namespace std;

RingBuffer *RingBuffer::GetInstance()
{
	static RingBuffer *_instance = NULL;
	if(NULL == _instance) {
		_instance = new RingBuffer(DEFAULT_SIZE, DEFAULT_NUM);
	}
	return _instance;
}

//该场景不需要线程保护

RingBuffer::RingBuffer(unsigned int _n_size, unsigned int _nmax)
{
	ring_buffer = NULL;
	n_size = _n_size;
	n_max = _nmax;
	write_pos = read_pos = 0;
}

RingBuffer::~RingBuffer()
{
	if(ring_buffer) {
		for(unsigned int i = 0; i < n_max; i++) {

			if(ring_buffer[i].buffer != NULL) {
				delete[] ring_buffer[i].buffer;
			}else {
				break;
			}
		}
	}
}

bool RingBuffer:: Init()
{
	bool ret = false;
	ring_buffer = new ring_buffer_t[n_size];
	if(ring_buffer) {
		for(unsigned int i = 0; i < n_max; i++) {
			ring_buffer[i].buffer = new unsigned char[n_size];
			if(NULL == ring_buffer[i].buffer) {
				break;
			}
			ring_buffer[i].status = RING_STATUS_EMPTY;
			ring_buffer[i].size = n_size;
		}
		ret = true;
	}
	return ret;
}

bool RingBuffer::PutRing(ring_buffer_t *_ring_buffer)
{
	if(_ring_buffer && _ring_buffer->size > n_max) {
		cout<<"invalid params"<<endl;
		return false;
	}
	write_pos = (write_pos + 1) > n_size ? 0 : (write_pos+1);
	ring_buffer_t *p = &ring_buffer[write_pos];
	memcpy(p->buffer, _ring_buffer->buffer, _ring_buffer->size);
	p->size = _ring_buffer->size;
	p->status = RING_STATUS_FULL;
	return true;
}
ring_buffer_t *RingBuffer::GetRing()
{
	read_pos = (read_pos + 1) > n_size ? 0 : (read_pos+1);
	ring_buffer_t *p = &ring_buffer[read_pos];
	if(p->status == RING_STATUS_EMPTY) {
		return NULL;
	}
	p->status = RING_STATUS_EMPTY;
	return p;
}

