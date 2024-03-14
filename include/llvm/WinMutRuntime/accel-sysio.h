#include <string>
#include <sstream>
#include "llvm/WinMutRuntime/logging/LogForMutTool.h"
#include "llvm/WinMutRuntime/logging/LogFilePrefix.h"

#include "llvm/WinMutRuntime/filesystem/LibCAPI.h"

#include <cstring>
// #include <cstdlib>

// 基类
class IOSystemCall {
public:
    virtual const char* getStr(){};
    virtual size_t getCount() {};
};

enum Call {
    Type_OpenCall,
    Type_ReadCall,
    Type_WriteCall,
    Type_CloseCall,
    Type_LseekCall,
};

// class OpenCall : public IOSystemCall {
// private:
//     std::string filename;
//     int flags;
//     int mode;

// public:
//     OpenCall(const std::string& filename, int flags, int mode) : filename(filename), flags(flags), mode(mode) {}

//     bool operator==(const OpenCall& other) const {
//         return filename == other.filename && flags == other.flags && mode == other.mode;
//     }

//     std::string getStr(){
//         std::stringstream ss;
//         ss << "OpenCall : " << filename << " " << flags << " " << mode;
//         return ss.str();
//     }

//     OpenCall(){}

//     void writeToFile(const char* filename){
//         int call_type = Call::Type_OpenCall;

//         char int_buf[sizeof(int)];

//         *(int *)int_buf = call_type;
//         writeToMutToolLogFile(filename, int_buf, sizeof(int));


//         int len = this->filename.length();
//         *(int *)int_buf = len;
//         writeToMutToolLogFile(filename, int_buf, sizeof(int));
//         writeToMutToolLogFile(filename, this->filename);


//         *(int *)int_buf = this->flags;
//         writeToMutToolLogFile(filename, int_buf, sizeof(int));

//         *(int *)int_buf = this->mode;
//         writeToMutToolLogFile(filename, int_buf, sizeof(int));
        
//     }

//     void readFromFile(int fd){
//         // 此时类型由外部调用函数判定，此处只负责读取属性

//         int len;
//         __accmut_libc_read(fd, &len, sizeof(int));

//         char* buf = (char *)malloc(len * sizeof(char));
//         __accmut_libc_read(fd, buf, len);

//         this->filename = buf;

//         __accmut_libc_read(fd, &this->flags, sizeof(int));
//         __accmut_libc_read(fd, &this->mode, sizeof(int));
//     }
// };

// class CloseCall : public IOSystemCall {
// private:
//     int fd;

//     int ret_result;
// public:
//     CloseCall(int ret_result, int fd) : ret_result(ret_result), fd(fd) {}
// };

// class ReadCall : public IOSystemCall {
// private:
//     size_t count;

// public:
//     ReadCall(size_t count) : count(count) {}

//     bool operator==(const ReadCall& other) const {
//         return count == other.count;
//     }

//     std::string getStr(){
//         std::stringstream ss;
//         ss << "ReadCall : " << count;
//         return ss.str();
//     }

//     ReadCall(){}

//     void writeToFile(const char* filename){
//         int call_type = Call::Type_ReadCall;

//         char int_buf[sizeof(int)];
//         char size_t_buf[sizeof(size_t)];

//         *(int *)int_buf = call_type;
//         writeToMutToolLogFile(filename, int_buf, sizeof(int));

//         *(size_t *)size_t_buf = this->count;
//         writeToMutToolLogFile(filename, size_t_buf, sizeof(size_t));
        
//     }

//     void readFromFile(int fd){
//         // 此时类型由外部调用函数判定，此处只负责读取属性
//         __accmut_libc_read(fd, &this->count, sizeof(size_t));
//     }
// };

class WriteCall : public IOSystemCall {
private:
    char* buf;
    size_t count;

public:
    WriteCall(const char* buf, size_t count) : count(count) {
        this->buf = new char[count + 1];
        memcpy(this->buf, buf, count);
        this->buf[count] = '\0';
    }

    ~WriteCall(){
        delete[] this->buf;
    }

    bool operator==(const WriteCall& other) const {
        if (this->count != other.count)
            return 0;

        for (int i = 0; i < this->count; ++i) {
            if (this->buf[i] != other.buf[i])
                return 0;
        }
        return 1;
    }

    const char* getStr(){
        return buf;
    }

    size_t getCount(){
        return this->count;
    }

    WriteCall(){}

    void writeToFile(const char* filename){
        int call_type = Call::Type_WriteCall;

        char int_buf[sizeof(int)];
        char size_t_buf[sizeof(size_t)];

        *(int *)int_buf = call_type;
        writeToMutToolLogFile(filename, int_buf, sizeof(int));

         *(size_t *)size_t_buf = this->count;
        writeToMutToolLogFile(filename, size_t_buf, sizeof(size_t));  

        writeToMutToolLogFile(filename, this->buf, this->count);

        
    }

    void readFromFile(int fd){
        // 此时类型由外部调用函数判定，此处只负责读取属性

        __accmut_libc_read(fd, &this->count, sizeof(size_t));

        this->buf = new char[this->count + 1];
        __accmut_libc_read(fd, buf, this->count);

        this->buf[this->count] = '\0';
        
    }
};

// class LseekCall : public IOSystemCall {
// private:
//     off_t offset;
//     int whence;

// public:
//     LseekCall(off_t offset, int whence) : offset(offset), whence(whence) {}

//     bool operator==(const LseekCall& other) const {
//         return offset == other.offset && whence == other.whence;
//     }

//     std::string getStr(){
//         std::stringstream ss;
//         ss << "LseekCall : " << offset << " " << whence;
//         return ss.str();
//     }

//     LseekCall(){}

//     void writeToFile(const char* filename){
//         int call_type = Call::Type_LseekCall;

//         char int_buf[sizeof(int)];
//         char size_t_buf[sizeof(size_t)];

//         *(int *)int_buf = call_type;
//         writeToMutToolLogFile(filename, int_buf, sizeof(int));

//         *(off_t *)size_t_buf = this->offset;
//         writeToMutToolLogFile(filename, size_t_buf, sizeof(off_t));

//         *(int *)int_buf = whence;
//         writeToMutToolLogFile(filename, int_buf, sizeof(int));
        
//     }

//     void readFromFile(int fd){
//         // 此时类型由外部调用函数判定，此处只负责读取属性
//         __accmut_libc_read(fd, &this->offset, sizeof(off_t));
//         __accmut_libc_read(fd, &this->whence, sizeof(int));
//     }
// };

int isOrinalLogging();

int isCheckingSysio();

void startOrinalLog();

void resetOrinalLog(int i);

void endOrinalLog();

void readOriginalLog();


// void logSysioOfOrinal_open(const char *pathname, int flags, int mode);

// void logSysioOfOrinal_read(size_t count);

void logSysioOfOrinal_write(int fd, const char *buf, size_t count);

// void logSysioOfOrinal_lseek(off_t offset, int whence);

// void checkSysioOfMutant_open(const char *pathname, int flags, int mode);

// void checkSysioOfMutant_read(size_t count);

void checkSysioOfMutant_write(int fd, const char *buf, size_t count);

// void checkSysioOfMutant_lseek(off_t offset, int whence);



// void log_fork_from_in_log_sysio(int mut_id);
void copy_log_sysio_of_fork_from(int mut_id);
void register_opened_file(int fd, const char* filepath);
