#ifndef LIM_BYTE_BUFFER_H
#define LIM_BYTE_BUFFER_H
#include <string>
#include <atomic>

namespace lim {
  //带引用计数的字节缓冲区(非线程安全)
  class ByteBuffer {
  public:
    ByteBuffer(int max_buffer_size = -1);
    ByteBuffer(int init_buffer_size, int max_buffer_size);
    ByteBuffer(const ByteBuffer& other);
    ByteBuffer &operator=(const ByteBuffer& other);
    virtual ~ByteBuffer();

  public:
    //清空缓冲区
    void Clear();
    //获取缓冲区大小
    int Capacity();
    //字符序列化缓冲区内容
    std::string ToString();

    //获取缓冲区可读字节数
    int ReadableBytes();
    //获取缓冲区可写字节数
    int WritableBytes();

    //skip指定可读字节数
    int SkipBytes(int size);
    /**
    *读取指定大小字节数
    * @param bytes 读缓冲区
    * @param size 读缓冲区大小
    * @return 返回实际读出的字节数
    */
    int ReadBytes(char *bytes, int size);
    /**
    *读取指定大小字节数
    * @param other 读缓冲区
    * @param size 读缓冲区大小,-1表示读所有数据
    * @return 返回实际读出的字节数
    */
    int ReadBytes(ByteBuffer &other, int size = -1);
		
    uint8_t ReadUInt8();
    void WriteUInt8(uint8_t value);
		
    uint16_t ReadUInt16();
    void WriteUInt16(uint16_t value);
		
    uint32_t ReadUInt32();
    void WriteUInt32(uint32_t value);
		
    uint64_t ReadUInt64();
    void WriteUInt64(uint64_t value);
		
    /**
    *写指定大小字节数
    * @param bytes 写缓冲区
    * @param size 写缓冲区大小
    * @return 返回实际写入的字节数
    */
    int WriteBytes(const char *bytes, int size);
    /**
    *写指定大小字节数
    * @param other 写缓冲区
    * @param size 写缓冲区大小,-1表示写所有数据
    * @return 返回实际写入的字节数
    */
    int WriteBytes(ByteBuffer &other, int size = -1);
    /**
    *从指定位置写入指定大小字节数
    * @param bytes 写缓冲区
    * @param bytes 写缓冲区
    * @param size 写缓冲区大小
    * @return 成功true,失败false
    */
    bool InsertWriteBytes(int position, const char *bytes, int size);

    /**
    *获取一行数据
    * @param line 返回的行数据
    * @param delim 分割字符串
    * @return 成功true,失败false
    */
    bool GetLine(std::string &line, const std::string &delim);
		
  private:
    /**
    *读网络字节次序数据
    * @param size 需要读取的字节数，size <= sizeof(uint64_t)
    * @return 返回网络字节次序数据
    */
    uint64_t ReadValue(int size);
    /**
    *写网络字节次序数据
    * @param value 需要写的字值
    * @param size value值的大小，size <= sizeof(uint64_t)
    */
    void WriteValue(uint64_t value, int size);

    //申请缓存区
    bool AllocateMemory(int memory_size);
		
  private:
    char *buffer_; /***缓存区***/
    int *buffer_read_index_; /***读偏移指针***/
    int *buffer_write_index_; /***写偏移指针***/
    int *buffer_size_; /***当前缓存区大小指针***/
    int *max_buffer_size_; /***最大缓存区大小指针***/
    std::atomic<int> *reference_count_; /***引用计数指针***/
  };
}
#endif

