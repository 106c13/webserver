#include <cstddef>
#include <iostream>
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
    if (!buf || len == 0)
        return;

    size_t cur_size = end_ - start_;
    size_t free_space = cap_ - cur_size;

    if (start_ > 0) {
        memmove(data_, data_ + start_, cur_size);
        start_ = 0;
        end_ = cur_size;
    }

    free_space = cap_ - end_;

    if (free_space < len) {
        size_t new_cap = cap_;
        while (new_cap - cur_size < len)
            new_cap += 1024;

        char* tmp = new char[new_cap];

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

void Buffer::append(const std::string& buf) {
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

bool Buffer::findCRLF(size_t& pos) const {
    const char* d = data_;
    size_t len = end_ - start_;

    for (size_t i = 0; i + 1 < len; ++i) {
        if (d[start_ + i] == '\r' && d[start_ + i + 1] == '\n') {
            pos = i;
            return true;
        }
    }
    return false;
}
