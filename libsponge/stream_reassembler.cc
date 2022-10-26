#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) :  _output(capacity), _capacity(capacity), eof_index(capacity) {
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
// 将字符串子串按顺序写入到输出流中
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    
    // 更新滑动窗口位置
    read_ptr = _output.bytes_read();
    string ndata = data;

    // 确定发送内容是否位于滑动窗口内，以及eof的位置
    if((index+data.size())>(read_ptr + _capacity)){
        ndata = data.substr(0,(read_ptr + _capacity-index));
    }else if(eof){
        eof_index = index + ndata.size();     
        eof_f = true;
    }
    
    // 判断此次发送是否已经存在，若存在进行更新
    auto it = stream.find(index);
    if((it != stream.end())){
        if(ndata.size() > it->second.size()){
            stream[it->first] = ndata;
        }       
    }else{
        stream.insert({index,ndata});
    }
    
    // 根据发送队列重组写入
    for(auto iter = stream.begin(); iter != stream.end(); ){
        size_t old_write = write_ptr;
        if(iter->first <= old_write){
            if((iter->first+iter->second.size())>old_write){
                write_ptr = iter->first+iter->second.size();
                string write_data(iter->second.substr(old_write - iter->first));
                _output.write(write_data);
                iter = stream.erase(iter);
            }else{
                iter++;
            }
        }else{
            break;
        }
    }   

    // 发送完毕后整理后续发送队列内容
    auto old_it = stream.begin();
    for(auto iter = stream.begin(); iter != stream.end(); ){
        if(iter != stream.begin()){
            if(iter->first < (old_it->first+old_it->second.size()))
            {
                if((iter->first+iter->second.size())<= (old_it->first+old_it->second.size()))
                {
                    iter = stream.erase(iter);
                }else{
                    string s = old_it->second + iter->second.substr((old_it->first+old_it->second.size())-iter->first);
                    old_it->second = s;
                    iter = stream.erase(iter);
                }

            }else{
                old_it = iter++;
            }        
        }else{
            old_it = iter++;
        }
    }

    if((write_ptr == eof_index) && eof_f){
        _output.end_input();
    }
}

// 统计以发出但还未被写入的字符数
size_t StreamReassembler::unassembled_bytes() const { 
    size_t unassem = 0;
    for(auto iter = stream.begin(); iter != stream.end(); iter++ ){
        if(iter->first > write_ptr){
            unassem += iter->second.size();
        }
    }
    return unassem; 
}

bool StreamReassembler::empty() const { 
    return {unassembled_bytes() == 0}; 
}
