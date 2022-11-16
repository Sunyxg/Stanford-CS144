#include "byte_stream.hh"
using namespace std;

ByteStream::ByteStream(const size_t capacity) { 
    b_size = capacity;
}

// 向字节流缓冲区写入数据data,并返回最终实际写入的数据长度
size_t ByteStream::write(const string &data) {
    size_t rem = remaining_capacity();
    size_t len = data.size();
    // 判断写入数据长度是否超出字节流缓冲区剩余可写入字节的大小
    if(len > rem){
        len = rem;
    }
    size_t i = 0;
    // 写入长度为len的字节大小
    while(i < len){
        bts.push_back(data[i]);
        i++;
        write_count++;
    }
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
// 从字节流缓冲区读出长度为len字节的数据
string ByteStream::peek_output(const size_t len) const {
    string s;    
    s.append(bts.begin(),bts.begin()+len);
    return s;
}

//! \param[in] len bytes will be removed from the output side of the buffer
// 从字节流缓冲区输出端移除长度为len字节的数据
void ByteStream::pop_output(const size_t len) { 
    size_t i = 0;
    while(i<len){
        bts.pop_front();
        i++;
        read_count ++;
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
// 从字节流缓冲区输出端读出长度为len字节的数据，如果缓冲区数据不足len则读出缓冲区所有数据
std::string ByteStream::read(const size_t len) {
    size_t length = len;
    // 判断缓冲区是否有len长度数据    
    if(length > buffer_size())
    {
        length = buffer_size();
    }
    string s = peek_output(length);
    pop_output(length);
    return s;
}

// 写入结束，置写入结束标志位true
void ByteStream::end_input() { end_flag = true;}
// 返回写入端写入是否结束
bool ByteStream::input_ended() const { return end_flag; }
// 当前可从缓冲区中读出的最大字节数量
size_t ByteStream::buffer_size() const { return bts.size(); }
// 当前缓冲区是否为空
bool ByteStream::buffer_empty() const { return bts.empty(); }
// 返回缓冲区是否处于eof状态（写入端写入结束，且字节流缓冲区为空）
bool ByteStream::eof() const { return (end_flag && buffer_empty()); }
// 返回已向缓冲区写入的字节的数量
size_t ByteStream::bytes_written() const { return write_count; }
// 返回已从缓冲区中读出的字节数量
size_t ByteStream::bytes_read() const { return read_count; }
// 返回字节流缓冲区剩余可写入字节的大小
size_t ByteStream::remaining_capacity() const { return {b_size - bts.size()}; }
