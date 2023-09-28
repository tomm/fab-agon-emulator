#ifndef MULTI_BUFFER_STREAM_H
#define MULTI_BUFFER_STREAM_H

#include <memory>
#include <vector>
#include <Stream.h>

#include "buffer_stream.h"
#include "types.h"

class MultiBufferStream : public Stream {
	public:
		MultiBufferStream(std::vector<std::shared_ptr<BufferStream>, psram_allocator<std::shared_ptr<BufferStream>>> buffers);
		int available();
		int read();
		int peek();
		size_t write(uint8_t b);
		void rewind();
	private:
		std::vector<std::shared_ptr<BufferStream>, psram_allocator<std::shared_ptr<BufferStream>>> buffers;
		std::shared_ptr<BufferStream> getBuffer();
		size_t currentBufferIndex = 0;
};

MultiBufferStream::MultiBufferStream(std::vector<std::shared_ptr<BufferStream>, psram_allocator<std::shared_ptr<BufferStream>>> buffers) : buffers(buffers) {
	// rewind all buffers, in case they've been used before
	rewind();
}

int MultiBufferStream::available() {
	auto buffer = getBuffer();
	if (buffer) {
		return buffer->available();
	}
	return 0;
}

int MultiBufferStream::read() {
	auto buffer = getBuffer();
	return buffer->read();
}

int MultiBufferStream::peek() {
	auto buffer = getBuffer();
	return buffer->peek();
}

size_t MultiBufferStream::write(uint8_t b) {
	// write is not supported
	return 0;
}

void MultiBufferStream::rewind() {
	for (auto buffer : buffers) {
		buffer->rewind();
	}
	currentBufferIndex = 0;
}

inline std::shared_ptr<BufferStream> MultiBufferStream::getBuffer() {
	while (currentBufferIndex < buffers.size() && !buffers[currentBufferIndex]->available()) {
		currentBufferIndex++;
	}
	if (currentBufferIndex >= buffers.size()) {
		return nullptr;
	}
	return buffers[currentBufferIndex];
}

#endif // MULTI_BUFFER_STREAM_H
