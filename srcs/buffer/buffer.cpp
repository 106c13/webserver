#include <cstddef>
#include <cstring>
#include "Buffer.h"

Buffer::Buffer() {
	data_ = new char[1024];
	cap_ = 1024;
	start_ = 0;
	end_ = 0;
}

Buffer::~Buffer() {
	delete[] data_;
}

Buffer::Buffer(const Buffer& src)
{
    cap_ = src.size() + 1024;
    data_ = new char[cap_];
    memcpy(data_, src.data(), src.size());

    start_ = 0;
    end_ = src.size();
}

Buffer& Buffer::operator=(const Buffer& src) {
    if (this == &src)
        return *this;

    delete[] data_;

    cap_ = src.cap_;
    data_ = new char[cap_];
    memcpy(data_, src.data(), src.size());
    start_ = 0;
    end_ = src.size();
    
    return *this;
}

size_t Buffer::size() const {
	return (end_ - start_);
}

bool Buffer::empty() const {
	return (end_ == start_);
}

const char* Buffer::data() const {
	return data_ + start_;
}

void Buffer::append(const char* buf, size_t len) {
    if (cap_ - end_ < len) {
        size_t new_cap = cap_ + len + 1024;
        char* tmp = new char[new_cap];

        size_t cur_size = end_ - start_;
        memcpy(tmp, data_ + start_, cur_size);

        delete[] data_;
        data_ = tmp;
        cap_ = new_cap;

        start_ = 0;
        end_ = cur_size;
    }

    memcpy(data_ + end_, buf, len);
    end_ += len;
}

void Buffer::append(const char* buf) {
    if (!buf)
        return;
    append(buf, std::strlen(buf));
}

void Buffer::append(const std::string& buf)
{
    if (buf.empty())
        return;
    append(buf.data(), buf.size());
}



void Buffer::consume(size_t n) {
	start_ += n;
	if (start_ == end_) {
		start_ = 0;
		end_ = 0;
	}
}

void Buffer::clear() {
    start_ = 0;
    end_ = 0;
}
