#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`


using namespace std;

ByteStream::ByteStream(const size_t capacity) { 
    b_size = capacity;
}

size_t ByteStream::write(const string &data) {
    size_t rem = remaining_capacity();
    size_t len = data.size();
    if(len > rem){
        len = rem;
    }
    size_t i = 0;
    while(i < len){
        bts.push_back(data[i]);
        i++;
        write_count++;
    }
     
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string s;    
    s.append(bts.begin(),bts.begin()+len);
    return s;
}

//! \param[in] len bytes will be removed from the output side of the buffer
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
std::string ByteStream::read(const size_t len) {
    size_t length = len;

    if(length > buffer_size())
    {
        length = buffer_size();
    }
    string s = peek_output(length);
    pop_output(length);

    return s;
}

void ByteStream::end_input() { end_flag = true;}

bool ByteStream::input_ended() const { return end_flag; }

size_t ByteStream::buffer_size() const { return bts.size(); }

bool ByteStream::buffer_empty() const { return bts.empty(); }

bool ByteStream::eof() const { return (end_flag && buffer_empty()); }

size_t ByteStream::bytes_written() const { return write_count; }

size_t ByteStream::bytes_read() const { return read_count; }

size_t ByteStream::remaining_capacity() const { return {b_size - bts.size()}; }
