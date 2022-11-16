#include "stream_reassembler.hh"
using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) :  _output(capacity), _capacity(capacity), eof_index(capacity) {
}

// 将字符串子串按顺序写入到输出流中
// data 此次接收到的TCPSegment中包含的字符串子串
// index 为子串中第一个字节的索引（在接收字节流中的位置）
// eof 标志为真时说明该子串为整个接收字节流的最后一个子串（即写入结束）
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    
    // 更新StreamReassembler窗口位置
    read_ptr = _output.bytes_read();
    string ndata = data;

    // 确定接收到的子串是否位于StreamReassembler窗口内
    if(index >(read_ptr + _capacity) || (index+data.size()) < write_ptr){
        return;
    }

    // 如果只有部分子串位于接收窗口，则只接收位于窗口内的部分，并确定eof是否位于窗口内
    if((index+data.size())>(read_ptr + _capacity)){
        ndata = data.substr(0,(read_ptr + _capacity-index));
    }else if(eof){
        eof_index = index + ndata.size();     
        eof_f = true;
    }
    
    // 判断此次接收的子串起始位置是否与StreamReassembler窗口内已有子串重叠
    auto it = stream.find(index);
    if((it != stream.end())){
        // 如果子串重叠则记录保存更长的子串
        if(ndata.size() > it->second.size()){
            stream[it->first] = ndata;
        }       
    }else{
        // 如果没有重叠则将子串插入StreamReassembler窗口
        stream.insert({index,ndata});
    }
    
    // 将StreamReassembler窗口子字符串进行重组
    // 遍历StreamReassembler窗口中待重组的子字符串
    for(auto iter = stream.begin(); iter != stream.end(); ){
        // 如果子字符串起始索引小于等于ByteStream将要写入的下一个字节的索引，则写入该子字符串
        if(iter->first <= write_ptr){
            // 判断子串中是否有内容还没被写入ByteStream
            if((iter->first+iter->second.size())>write_ptr){
                // 分割出子串中没有被写入ByteStream的部分
                string write_data(iter->second.substr(write_ptr - iter->first));
                _output.write(write_data);
                write_ptr = _output.bytes_written();
                
            }
            // 删除已写入ByteStream的子字符串
            iter = stream.erase(iter);
        }else{
            // 如果子字符串起始索引大于ByteStream将要写入的下一个字节的索引，跳出循环
            break;
        }
    }   

    //向ByteStream写入完毕后整理后续StreamReassembler窗口中待重组的子字符串
    auto old_it = stream.begin();
    for(auto iter = stream.begin(); iter != stream.end(); ){
        // 跳过第一个子串
        if(iter == stream.begin()){
            iter++;
        }else{
            // 判断后一个相邻两个子串是否重合
            if(iter->first < (old_it->first+old_it->second.size()))
            {
                // 如果前一个子串包含后一个子串则直接删除后面子串
                if((iter->first+iter->second.size())<= (old_it->first+old_it->second.size()))
                {
                    iter = stream.erase(iter);
                }else{
                    // 如果相邻子串有部分重合，则将两个子串合并为一个子串
                    string s = old_it->second + iter->second.substr((old_it->first+old_it->second.size())-iter->first);
                    old_it->second = s;
                    iter = stream.erase(iter);
                }

            }else{
                old_it = iter++;
            } 
        }
    }

    // 最后一个子串已写入，且eof为true，则通知接收方Bytestream字节流接收完毕
    if((write_ptr == eof_index) && eof_f){
        _output.end_input();
    }
}

// 统计接收端已经收到但仍未成功重组的字节数量
size_t StreamReassembler::unassembled_bytes() const { 
    size_t unassem = 0;
    for(auto iter = stream.begin(); iter != stream.end(); iter++ ){
        if(iter->first > write_ptr){
            unassem += iter->second.size();
        }
    }
    return unassem; 
}

// 判断待重组的字节数量是否为0
bool StreamReassembler::empty() const { 
    return {unassembled_bytes() == 0}; 
}
