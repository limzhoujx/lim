#include <lim/base/byte_buffer.h>
#include <lim/base/string_utils.h>
#include <assert.h>
#include <string.h>

namespace lim {
	ByteBuffer::ByteBuffer(int max_buffer_size) {
		if (max_buffer_size == -1) {
			max_buffer_size = ((unsigned)(-1) >> 1);
		}

		int init_buffer_size = (max_buffer_size > 1024 ? 1024 : max_buffer_size);
		
		buffer_read_index_ = new int(0);
		buffer_write_index_ = new int(0);
		buffer_size_ = new int(init_buffer_size);
		max_buffer_size_ = new int(max_buffer_size);
		buffer_ = new char[*buffer_size_];
		reference_count_ = new std::atomic<int>(1);
	}
	
	ByteBuffer::ByteBuffer(int init_buffer_size, int max_buffer_size) {
		assert(init_buffer_size > 0 && init_buffer_size <= max_buffer_size);

		buffer_read_index_ = new int(0);
		buffer_write_index_ = new int(0);
		buffer_size_ = new int(init_buffer_size);
		max_buffer_size_ = new int(max_buffer_size);
		buffer_ = new char[*buffer_size_];
		reference_count_ = new std::atomic<int>(1);
	}

	ByteBuffer::ByteBuffer(const ByteBuffer &other)     {
		buffer_read_index_ = other.buffer_read_index_;
		buffer_write_index_ = other.buffer_write_index_;
		buffer_size_ = other.buffer_size_;
		max_buffer_size_ = other.max_buffer_size_;
		buffer_ = other.buffer_;
		
		reference_count_ = other.reference_count_;
		(*reference_count_) ++;
	}

	ByteBuffer &ByteBuffer::operator =(const ByteBuffer& other) {
		if (this == &other) {
			return *this;
		}

		if(--(*reference_count_) <= 0) {
			delete[] buffer_read_index_;
			delete[] buffer_write_index_;
			delete[] buffer_size_;
			delete[] max_buffer_size_;			
			delete[] buffer_;			
			delete reference_count_;	
		}

		buffer_read_index_ = other.buffer_read_index_;
		buffer_write_index_ = other.buffer_write_index_;
		buffer_size_ = other.buffer_size_;
		max_buffer_size_ = other.max_buffer_size_;
		buffer_ = other.buffer_;
		
		reference_count_ = other.reference_count_;
		(*reference_count_) ++;
		
		return *this;
	}
	
	ByteBuffer::~ByteBuffer() {
		if (--(*reference_count_) <= 0) {
			delete[] buffer_read_index_;
			delete[] buffer_write_index_;
			delete[] buffer_size_;
			delete[] max_buffer_size_;			
			delete[] buffer_;
			delete reference_count_;	
		}
	}

	//清空缓冲区
	int ByteBuffer::Capacity() {
		return *max_buffer_size_;
	}

	//获取缓冲区大小
	void ByteBuffer::Clear() {
		*buffer_read_index_ = *buffer_write_index_ = 0;
	}

	//字符序列化缓冲区内容
	std::string ByteBuffer::ToString() {
		std::string value(buffer_ + *buffer_read_index_, ReadableBytes());
		return value;
	}
	
	//获取缓冲区可读字节数
	int ByteBuffer::ReadableBytes() {
		return (*buffer_write_index_ - *buffer_read_index_);
	}

	//获取缓冲区可写字节数
	int ByteBuffer::WritableBytes() {
		return (*max_buffer_size_ - *buffer_write_index_);
	}

	//申请缓存区
	bool ByteBuffer::AllocateMemory(int memory_size) {
		int allocate_size = *buffer_size_;
		while (true) {
			allocate_size = ((allocate_size * 3 / 2) <= *max_buffer_size_ ? (allocate_size * 3 / 2) : *max_buffer_size_);
			if (allocate_size >= memory_size)
				break;
			else if (allocate_size == *max_buffer_size_)
				return false;
		}

		char *allocate_buffer = new char[allocate_size];
		memcpy(allocate_buffer, buffer_, *buffer_size_);

		buffer_ = allocate_buffer;
		*buffer_size_ = allocate_size;

		return true;
	}

  /**
  *读网络字节次序数据
  * @param size 需要读取的字节数，size <= sizeof(uint64_t)
  * @return 返回网络字节次序数据
  */
	uint64_t ByteBuffer::ReadValue(int size) {
		if (size > ReadableBytes() || size > sizeof(uint64_t)) {
			return -1;
		}

		uint64_t value = 0;
		for (int i = 0; i < size; i++) {
			value = (value << i*8) + (uint8_t)(*(buffer_ + *buffer_read_index_ + i));
		}

		*buffer_read_index_ += size;
		return value;
	}

  /**
  *写网络字节次序数据
  * @param value 需要写的字值
  * @param size value值的大小，size <= sizeof(uint64_t)
  */
	void ByteBuffer::WriteValue(uint64_t value, int size) {
		if (size > WritableBytes() || size > sizeof(uint64_t)) {
			return;
		}

		for (int i = size - 1; i >= 0; i--) {
			*(buffer_ + *buffer_write_index_) = (char)((value >> i*8)&0xff);
			*buffer_write_index_ += 1;
		}
	}
	
	uint8_t ByteBuffer::ReadUInt8() {
		return (uint8_t)ReadValue(sizeof(uint8_t));
	}

	void ByteBuffer::WriteUInt8(uint8_t value) {
		WriteValue(value, sizeof(uint8_t));
	}

	uint16_t ByteBuffer::ReadUInt16() {
		return (uint16_t)ReadValue(sizeof(uint16_t));
	}

	void ByteBuffer::WriteUInt16(uint16_t value) {
		WriteValue(value, sizeof(uint16_t));
	}

	uint32_t ByteBuffer::ReadUInt32() {
		return (uint32_t)ReadValue(sizeof(uint32_t));
	}

	void ByteBuffer::WriteUInt32(uint32_t value) {
		WriteValue(value, sizeof(uint32_t));
	}
	
	uint64_t ByteBuffer::ReadUInt64() {
		return ReadValue(sizeof(uint64_t));
	}

	void ByteBuffer::WriteUInt64(uint64_t value) {
		WriteValue(value, sizeof(uint64_t));
	}
	
	//skip指定可读字节数
	int ByteBuffer::SkipBytes(int size) {
		if (size <= 0) {
			return 0;
		}

		int length = size <= ReadableBytes() ? size : ReadableBytes();
		*buffer_read_index_ += length;
		return length;
	}
	
  /**
  *读取指定大小字节数
  * @param bytes 读缓冲区
  * @param size 读缓冲区大小
  * @return 返回实际读出的字节数
  */
	int ByteBuffer::ReadBytes(char *bytes, int size) {
		if (size <= 0) {
			return 0;
		}

		int length = (size <= ReadableBytes() ? size : ReadableBytes());
		memcpy(bytes, buffer_ + *buffer_read_index_, length);
		*buffer_read_index_ += length;

		return length;
	}
	
  /**
  *读取指定大小字节数
  * @param other 读缓冲区
  * @param size 读缓冲区大小,-1表示读所有数据
  * @return 返回实际读出的字节数
  */
	int ByteBuffer::ReadBytes(ByteBuffer &other, int size) {
		int length = other.WritableBytes() <= ReadableBytes() ? other.WritableBytes() : ReadableBytes();
		if (size != -1 && size > 0) {
			length = (length < size ? length : size);
		}
		other.WriteBytes(buffer_ + *buffer_read_index_, length);
		return length;
	}

  /**
  *写指定大小字节数
  * @param bytes 写缓冲区
  * @param size 写缓冲区大小
  * @return 返回实际写入的字节数
  */
	int ByteBuffer::WriteBytes(const char *bytes, int size) {
		int length = (WritableBytes() < size ? WritableBytes() : size);
		if (*buffer_write_index_ + length > *buffer_size_) {
			AllocateMemory(*buffer_write_index_ + length);
		}
		
		memcpy(buffer_ + *buffer_write_index_, bytes, length);
		*buffer_write_index_ += length;

		return length;
	}

  /**
  *写指定大小字节数
  * @param other 写缓冲区
  * @param size 写缓冲区大小,-1表示写所有数据
  * @return 返回实际写入的字节数
  */
	int ByteBuffer::WriteBytes(ByteBuffer &other, int size) {
		int length = (WritableBytes() < other.ReadableBytes() ? WritableBytes() : other.ReadableBytes());
		if (size != -1 && size > 0) {
			length = (length < size ? length : size);
		}
		
		if (*buffer_write_index_ + length > *buffer_size_) {
			AllocateMemory(*buffer_write_index_ + length);
		}

		other.ReadBytes(buffer_ + *buffer_write_index_, length);
		*buffer_write_index_ += length;

		return length;
	}

  /**
  *从指定位置写入指定大小字节数
  * @param bytes 写缓冲区
  * @param bytes 写缓冲区
  * @param size 写缓冲区大小
  * @return 成功true,失败false
  */
	bool ByteBuffer::InsertWriteBytes(int position, const char *bytes, int size) {
		if (position < 0 || position > ReadableBytes() || size <= 0 || WritableBytes() < size) {
			return false;
		}

		if (*buffer_write_index_ + size > *buffer_size_) {
			AllocateMemory(*buffer_write_index_ + size);
		}
		
		memmove(buffer_ + *buffer_read_index_ + position + size, buffer_ + *buffer_read_index_ + position, ReadableBytes() - position);
		memcpy(buffer_ + *buffer_read_index_ + position, bytes, size);
		*buffer_write_index_ += size;

		return true;
	}

  /**
  *获取一行数据
  * @param line 返回的行数据
  * @param delim 分割字符串
  * @return 成功true,失败false
  */
	bool ByteBuffer::GetLine(std::string &line, const std::string &delim) {
		char* delim_position = (char*)memmem(buffer_ + *buffer_read_index_, ReadableBytes(), delim.c_str(), delim.length());
		if (delim_position == NULL) {
			return false;
		}

		int line_length = delim_position - (buffer_ + *buffer_read_index_);
		line = std::string(buffer_ + *buffer_read_index_, line_length);
		SkipBytes(line_length + delim.length());
		return true;
	}
}
