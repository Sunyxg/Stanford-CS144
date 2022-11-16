#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <string>
#include <map>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // 已重组的有序字节流写入的接收端的ByteStream
    ByteStream _output;  
    // StreamReassembler的容量（其等于接收方ByteStream的容量）
    size_t _capacity = 0;    
    // 接收端ByteStream将要读取的下一个字节的索引
    size_t read_ptr = 0;
    // 接收端ByteStream将要写入的下一个字节的索引（第一个有待重组的字节的索引）
    size_t write_ptr = 0;
    // 接收端接收到字节流的结束位置索引
    size_t eof_index = 0;
    // 如果接收端已经接收到最后一个子字符串的最后一个字节，置true
    bool eof_f = false;
    // 使用map（根据index从小到大排列）保存尚未重组的子字符串
    std::map<size_t,std::string> stream{};

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring 子字符串
    //! \param index indicates the index (place in sequence) of the first byte in `data`  
    //  data 中第一个字节的索引（顺序中的位置）
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    //  data 的最后一个字节是否是整个数据流的最后一个字节
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
