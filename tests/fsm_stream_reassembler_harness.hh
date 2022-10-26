#ifndef SPONGE_FSM_STREAM_REASSEMBLER_HARNESS_HH
#define SPONGE_FSM_STREAM_REASSEMBLER_HARNESS_HH

#include "byte_stream.hh"
#include "stream_reassembler.hh"
#include "util.hh"

#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

// 报错
class ReassemblerExpectationViolation : public std::runtime_error {
  public:
    ReassemblerExpectationViolation(const std::string msg) : std::runtime_error(msg) {}
};

struct ReassemblerTestStep {
    virtual std::string to_string() const { return "ReassemblerTestStep"; }
    virtual void execute(StreamReassembler &) const {}
    virtual ~ReassemblerTestStep() {}
};

struct ReassemblerAction : public ReassemblerTestStep {
    std::string to_string() const { return "Action:      " + description(); }
    virtual std::string description() const { return "description missing"; }
    virtual void execute(StreamReassembler &) const {}
    virtual ~ReassemblerAction() {}
};

struct ReassemblerExpectation : public ReassemblerTestStep {
    std::string to_string() const { return "Expectation: " + description(); }
    virtual std::string description() const { return "description missing"; }
    virtual void execute(StreamReassembler &) const {}
    virtual ~ReassemblerExpectation() {}
};

// 判断字节流缓存中内容与预期内容是否一致
struct BytesAvailable : public ReassemblerExpectation {
    std::string _bytes;
    
    // 移动构造
    BytesAvailable(std::string &&bytes) : _bytes(std::move(bytes)) {}
    std::string description() const {
        std::ostringstream ss;
        ss << "stream_out().buffer_size() returned " << _bytes.size() << ", and stream_out().read(" << _bytes.size()
           << ") returned the string \"" << _bytes << "\"";
        return ss.str();
    }

    void execute(StreamReassembler &reassembler) const {
        if (reassembler.stream_out().buffer_size() != _bytes.size()) {
            std::ostringstream ss;
            ss << "The reassembler was expected to have `" << _bytes.size() << "` bytes available, but there were `"
               << reassembler.stream_out().buffer_size() << "`";
            throw ReassemblerExpectationViolation(ss.str());
        }
        std::string data = reassembler.stream_out().read(_bytes.size());
        if (data != _bytes) {
            std::ostringstream ss;
            ss << "The reassembler was expected to have  bytes \"" << _bytes << "\", but there were \"" << data << "\"";
            throw ReassemblerExpectationViolation(ss.str());
        }
    }
};

// 判断已写字节流是否等于bytes
struct BytesAssembled : public ReassemblerExpectation {
    size_t _bytes;

    BytesAssembled(size_t bytes) : _bytes(bytes) {}
    std::string description() const {
        std::ostringstream ss;
        ss << "net bytes assembled = " << _bytes;
        return ss.str();
    }

    void execute(StreamReassembler &reassembler) const {
        if (reassembler.stream_out().bytes_written() != _bytes) {
            std::ostringstream ss;
            ss << "The reassembler was expected to have `" << _bytes << "` total bytes assembled, but there were `"
               << reassembler.stream_out().bytes_written() << "`";
            throw ReassemblerExpectationViolation(ss.str());
        }
    }
};

// 判断已写入但尚未重新组装的子字符串字节数是否等于bytes
struct UnassembledBytes : public ReassemblerExpectation {
    size_t _bytes;

    UnassembledBytes(size_t bytes) : _bytes(bytes) {}
    std::string description() const {
        std::ostringstream ss;
        ss << "bytes not assembled = " << _bytes;
        return ss.str();
    }

    void execute(StreamReassembler &reassembler) const {
        if (reassembler.unassembled_bytes() != _bytes) {
            std::ostringstream ss;
            ss << "The reassembler was expected to have `" << _bytes << "` bytes not assembled, but there were `"
               << reassembler.unassembled_bytes() << "`";
            throw ReassemblerExpectationViolation(ss.str());
        }
    }
};

// 判断字节流的传输是否结束
struct AtEof : public ReassemblerExpectation {
    AtEof() {}
    std::string description() const {
        std::ostringstream ss;
        ss << "at EOF";
        return ss.str();
    }

    void execute(StreamReassembler &reassembler) const {
        if (not reassembler.stream_out().eof()) {
            std::ostringstream ss;
            ss << "The reassembler was expected to be at EOF, but was not";
            throw ReassemblerExpectationViolation(ss.str());
        }
    }
};

// 判断字节流的传输是否还没结束
struct NotAtEof : public ReassemblerExpectation {
    NotAtEof() {}
    std::string description() const {
        std::ostringstream ss;
        ss << "not at EOF";
        return ss.str();
    }

    void execute(StreamReassembler &reassembler) const {
        if (reassembler.stream_out().eof()) {
            std::ostringstream ss;
            ss << "The reassembler was expected to **not** be at EOF, but was";
            throw ReassemblerExpectationViolation(ss.str());
        }
    }
};

// 分段传输
struct SubmitSegment : public ReassemblerAction {
    std::string _data;
    size_t _index;
    bool _eof{false};

    SubmitSegment(std::string data, size_t index) : _data(data), _index(index) {}

    // 更改eof的值
    SubmitSegment &with_eof(bool eof) {
        _eof = eof;
        return *this;
    }

    std::string description() const {
        std::ostringstream ss;
        ss << "substring submitted with data \"" << _data << "\", index `" << _index << "`, eof `"
           << std::to_string(_eof) << "`";
        return ss.str();
    }

    // 调用StreamReassembler.push_substring传输数据
    void execute(StreamReassembler &reassembler) const { reassembler.push_substring(_data, _index, _eof); }
};

//重组器测试
class ReassemblerTestHarness {
    StreamReassembler reassembler;
    std::vector<std::string> steps_executed;

  public:
    ReassemblerTestHarness(const size_t capacity) : reassembler(capacity), steps_executed() {
        steps_executed.emplace_back("Initialized (capacity = " + std::to_string(capacity) + ")");
    }

    void execute(const ReassemblerTestStep &step) {
        try {
            step.execute(reassembler);
            steps_executed.emplace_back(step.to_string());
        } catch (const ReassemblerExpectationViolation &e) {
            std::cerr << "Test Failure on expectation:\n\t" << step.to_string();
            std::cerr << "\n\nFailure message:\n\t" << e.what();
            std::cerr << "\n\nList of steps that executed successfully:";
            for (const std::string &s : steps_executed) {
                std::cerr << "\n\t" << s;
            }
            std::cerr << std::endl << std::endl;
            throw e;
        } catch (const std::exception &e) {
            std::cerr << "Test Failure on expectation:\n\t" << step.to_string();
            std::cerr << "\n\nFailure message:\n\t" << e.what();
            std::cerr << "\n\nList of steps that executed successfully:";
            for (const std::string &s : steps_executed) {
                std::cerr << "\n\t" << s;
            }
            std::cerr << std::endl << std::endl;
            throw ReassemblerExpectationViolation("The test caused your implementation to throw an exception!");
        }
    }
};

#endif  // SPONGE_FSM_STREAM_REASSEMBLER_HARNESS_HH
