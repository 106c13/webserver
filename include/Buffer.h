#ifndef BUFFER_H
#define BUFFER_H

class Buffer {
	private:
		char*	data_;
		size_t	cap_;
		size_t	start_;
		size_t	end_;

	public:
		Buffer();
		Buffer(const Buffer& src);
		~Buffer();

		Buffer& operator=(const Buffer& src);

		size_t		size() const;
		bool		empty() const;

		const char*	data() const;


		void		append(const char* buf, size_t len);
		void		consume(size_t n);
		void		clear();
};

#endif
