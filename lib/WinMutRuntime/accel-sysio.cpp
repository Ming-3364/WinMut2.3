#include "llvm/WinMutRuntime/accel-sysio.h"
#include <vector>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

static int origin_logging = 0;  // 0 for false, 1 for true

static char output_filename[1000];
static char output_filename_bin[1000];

// IOSystemCall vector
static std::vector<IOSystemCall*> call_seq;  // Call sequences

int isOrinalLogging(){
    return origin_logging;
}

void startOrinalLog(){
    origin_logging = 1;
}

void resetOrinalLog(int i){
    call_seq.clear();

    sprintf(output_filename, "call_seq_%d", i);
    sprintf(output_filename_bin, "call_seq_%d_bin", i);

    writeToMutToolLogFile("debug", output_filename);
    writeToMutToolLogFile("debug", "\n");
}

void endOrinalLog(){
    origin_logging = 0;
    writeToMutToolLogFile("debug", "endOrinalLog\n");
}

// void writeOriginalLog(){
//     for (auto c : call_seq){
//         writeToMutToolLogFile("debug", "\t");
//         writeToMutToolLogFile("debug", c->getStr().c_str());
//         writeToMutToolLogFile("debug", "\n");
//     }
// }

void readOriginalLog(){
    if (origin_logging)
        return;

    char buff[MAXPATHLEN];
    strcpy(buff, getMutToolLogFilePrefix());
    strcat(buff, "call_seq_2_bin");

    int fd = __accmut_libc_open(buff, O_RDONLY);

    int call_type;
    while (__accmut_libc_read(fd, &call_type, sizeof(int)) != -1)
    {
        switch (call_type)
        {
        case Call::Type_OpenCall:
        {
            OpenCall* ptr = new OpenCall();
            ptr->readFromFile(fd);
            call_seq.push_back(ptr);
            break;
        }
        case Call::Type_ReadCall:
        {
            ReadCall* ptr = new ReadCall();
            ptr->readFromFile(fd);
            call_seq.push_back(ptr);
            break;
        }
        case Call::Type_WriteCall:
        {
            WriteCall* ptr = new WriteCall();
            ptr->readFromFile(fd);
            call_seq.push_back(ptr);
            break;
        }
        case Call::Type_LseekCall:
        {
            LseekCall* ptr = new LseekCall();
            ptr->readFromFile(fd);
            call_seq.push_back(ptr);
            break;
        }
        default:
            writeToMutToolLogFile("debug", "read call seq from binary file error\n");
            exit(-1);
            break;
        }
        writeToMutToolLogFile("debug", "read: ");
        writeToMutToolLogFile("debug", call_seq[call_seq.size()-1]->getStr().c_str());
        writeToMutToolLogFile("debug", "\n");
    }

    for (auto c : call_seq){
        writeToMutToolLogFile("debug", "\t");
        writeToMutToolLogFile("debug", c->getStr().c_str());
        writeToMutToolLogFile("debug", "\n");
    }
    
}



void logSysioOfOrinal_open(const char* pathname, int flags, int mode){
    // 内存记录
    OpenCall* call_ptr = new OpenCall(pathname, flags, mode);
    call_seq.push_back(call_ptr);

    call_ptr->writeToFile(output_filename_bin);

    // 文件输出
    char outbuf[1000];
    // writeToMutToolLogFile("debug", "open function called\n");
    writeToMutToolLogFile(output_filename, "open ");
    writeToMutToolLogFile(output_filename, pathname);
    sprintf(outbuf, " %d %d\n", flags, mode);
    writeToMutToolLogFile(output_filename, outbuf);

    // 内存记录 · 校验和    关于校验和是否能降低成本，可以先统计字符串的长度

    // 文件输出 · 校验和

}

void logSysioOfOrinal_read(size_t count){
    // 内存记录
    ReadCall* call_ptr = new ReadCall(count);
    call_seq.push_back(call_ptr);

    call_ptr->writeToFile(output_filename_bin);


    // 文件输出
    char outbuf[1000];
    writeToMutToolLogFile(output_filename, "read ");
    sprintf(outbuf, " %ld\n", count);
    writeToMutToolLogFile(output_filename, outbuf);


    // 内存记录 · 校验和

    // 文件输出 · 校验和

}

void logSysioOfOrinal_write(const char* buf, size_t count){
    // 内存记录
    WriteCall* call_ptr = new WriteCall(buf, count);
    call_seq.push_back(call_ptr);

    call_ptr->writeToFile(output_filename_bin);

    // 文件输出
    char outbuf[1000];
    writeToMutToolLogFile(output_filename, "write ");
    writeToMutToolLogFile(output_filename, buf);
    sprintf(outbuf, " %ld\n", count);
    writeToMutToolLogFile(output_filename, outbuf);

    // 内存记录 · 校验和

    // 文件输出 · 校验和

}

void logSysioOfOrinal_lseek(off_t offset, int whence){
    // 内存记录
    LseekCall* call_ptr = new LseekCall(offset, whence);
    call_seq.push_back(call_ptr);

    call_ptr->writeToFile(output_filename_bin);

    // 文件输出
    char outbuf[1000];
    writeToMutToolLogFile(output_filename, "lseek ");
    sprintf(outbuf, " %ld %d\n", offset, whence);
    writeToMutToolLogFile(output_filename, outbuf);

    // 内存记录 · 校验和

    // 文件输出 · 校验和

}


// 子进程中的sisio和原始进程中的进行对比

unsigned long long cur_sysio_call= 0;

void checkSysioOfMutant_open(const char* pathname, int flags, int mode){
    // 直接对比内容
    OpenCall openCall(pathname, flags, mode);
    if (!(*(OpenCall*)call_seq[cur_sysio_call++] == openCall)){
        writeToMutToolLogFile("debug", "killed : ");
        writeToMutToolLogFile("debug", "ori : ");
        writeToMutToolLogFile("debug", call_seq[cur_sysio_call-1]->getStr().c_str());
        writeToMutToolLogFile("debug", " ");
        writeToMutToolLogFile("debug", "mut : ");
        writeToMutToolLogFile("debug", openCall.getStr().c_str());
        writeToMutToolLogFile("debug", "\n");


        writeToMutToolLogFile("proc_tree", "killed ");
        exit(42);
    }
    // 对比校验和？
}

void checkSysioOfMutant_read(size_t count){
    // 直接对比内容
    ReadCall readCall(count);
    if (!(*(ReadCall*)call_seq[cur_sysio_call++] == readCall)){

        writeToMutToolLogFile("debug", "killed : ");
        writeToMutToolLogFile("debug", "ori : ");
        writeToMutToolLogFile("debug", call_seq[cur_sysio_call-1]->getStr().c_str());
        writeToMutToolLogFile("debug", " ");
        writeToMutToolLogFile("debug", "mut : ");
        writeToMutToolLogFile("debug", readCall.getStr().c_str());
        writeToMutToolLogFile("debug", "\n");
        

        writeToMutToolLogFile("proc_tree", "killed ");
        exit(42);
    }

    // 对比校验和？
}

void checkSysioOfMutant_write(const char* buf, size_t count){
    // 直接对比内容
    WriteCall writeCall(buf, count);
    if (!(*(WriteCall*)call_seq[cur_sysio_call++] == writeCall)){

        writeToMutToolLogFile("debug", "killed : ");
        writeToMutToolLogFile("debug", "ori : ");
        writeToMutToolLogFile("debug", call_seq[cur_sysio_call-1]->getStr().c_str());
        writeToMutToolLogFile("debug", " ");
        writeToMutToolLogFile("debug", "mut : ");
        writeToMutToolLogFile("debug", writeCall.getStr().c_str());
        writeToMutToolLogFile("debug", "\n");


        writeToMutToolLogFile("proc_tree", "killed ");
        exit(42);
    }

    // 对比校验和？
}

void checkSysioOfMutant_lseek(off_t offset, int whence){
    // 直接对比内容
    LseekCall lseekCall(offset, whence);
    if (!(*(LseekCall*)call_seq[cur_sysio_call++] == lseekCall)){
        writeToMutToolLogFile("debug", "killed : ");
        writeToMutToolLogFile("debug", "ori : ");
        writeToMutToolLogFile("debug", call_seq[cur_sysio_call-1]->getStr().c_str());
        writeToMutToolLogFile("debug", " ");
        writeToMutToolLogFile("debug", "mut : ");
        writeToMutToolLogFile("debug", lseekCall.getStr().c_str());
        writeToMutToolLogFile("debug", "\n");

        writeToMutToolLogFile("proc_tree", "killed ");
        exit(42);
    }

    // 对比校验和？
}



