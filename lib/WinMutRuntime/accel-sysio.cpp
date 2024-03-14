#include "llvm/WinMutRuntime/accel-sysio.h"
#include <vector>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sstream>
#include <map>
#include "llvm/WinMutRuntime/mutations/MutationIDDecl.h"

#include "llvm/Transforms/WinMut/DebugMacro.h"

// #define DO_NOT_LOG_AND_CHECK_STDERR

#define OUTPUT_SYSIO_FOR_DEBUG

static std::map<int, std::string> openedFileMap_fd2path = {
    {1, "stdoutcopy"},
    {2, "stderrcopy"}
};
static int fd_dev_null;

static int origin_logging = 0;  // 0 for false, 1 for true
static int check_sysio = 0; // 同上

static char output_filename[1000];
static char output_filename_bin[1000];

// IOSystemCall vector
static std::vector<IOSystemCall*> call_seq;  // Call sequences

int isOrinalLogging(){
    return origin_logging;
}

int isCheckingSysio(){
    return check_sysio;
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
    check_sysio = 1;
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
    strcat(buff, "call_seq_1_bin");

    int fd = __accmut_libc_open(buff, O_RDONLY);
    if (fd == -1) {
        writeToMutToolLogFile("debug", "open call_seq_1_bin error: check if that exist!\n");
        check_sysio = 0;
        return;
        // exit(-1);
    }

    int call_type;
    while (__accmut_libc_read(fd, &call_type, sizeof(int)) != 0)
    {
        // std::ostringstream oss;
        // oss << "call_type: " << call_type << " ";
        // writeToMutToolLogFile("debug", oss.str().c_str());
        switch (call_type)
        {
        // case Call::Type_OpenCall:
        // {
        //     OpenCall* ptr = new OpenCall();
        //     ptr->readFromFile(fd);
        //     call_seq.push_back(ptr);
        //     break;
        // }
        // case Call::Type_ReadCall:
        // {
        //     ReadCall* ptr = new ReadCall();
        //     ptr->readFromFile(fd);
        //     call_seq.push_back(ptr);
        //     break;
        // }
        case Call::Type_WriteCall:
        {
            WriteCall* ptr = new WriteCall();
            ptr->readFromFile(fd);
            call_seq.push_back(ptr);
            break;
        }
        // case Call::Type_LseekCall:
        // {
        //     LseekCall* ptr = new LseekCall();
        //     ptr->readFromFile(fd);
        //     call_seq.push_back(ptr);
        //     break;
        // }
        default:
            writeToMutToolLogFile("debug", "read call seq from binary file error\n");
            exit(233);
            break;
        }
        writeToMutToolLogFile("debug", "\n{\n");
        writeToMutToolLogFile("debug", call_seq[call_seq.size()-1]->getStr(), call_seq[call_seq.size()-1]->getCount());
        writeToMutToolLogFile("debug", "\n}\n");
    }
    
    #ifdef OUTPUT_SYSIO_FOR_DEBUG

    writeToMutToolLogFile("debug", "\n========= dump =========\n");
    int idx = 0;
    for (auto c : call_seq){
        std::ostringstream oss;
        oss << "\n{SYSIO_LOG_BEGIN\n"
            << "INDEX: " << idx++ << "\n"
            << "TYPE: write\n"
            << "CONTENT: \n";
        writeToMutToolLogFile("debug", oss.str().c_str());
        writeToMutToolLogFile("debug", c->getStr(), c->getCount());
        oss.str("");
        oss << "\n}SYSIO_LOG_END\n";
        writeToMutToolLogFile("debug", oss.str().c_str());
    }
    writeToMutToolLogFile("debug", "\n========= dump end======\n");
    #endif
    
}



// void logSysioOfOrinal_open(const char* pathname, int flags, int mode){
//     // 内存记录
//     OpenCall* call_ptr = new OpenCall(pathname, flags, mode);
//     call_seq.push_back(call_ptr);

//     call_ptr->writeToFile(output_filename_bin);

//     // 文件输出
//     char outbuf[1000];
//     // writeToMutToolLogFile("debug", "open function called\n");
//     writeToMutToolLogFile(output_filename, "open ");
//     writeToMutToolLogFile(output_filename, pathname);
//     sprintf(outbuf, " %d %d\n", flags, mode);
//     writeToMutToolLogFile(output_filename, outbuf);

//     // 内存记录 · 校验和    关于校验和是否能降低成本，可以先统计字符串的长度

//     // 文件输出 · 校验和

// }

// void logSysioOfOrinal_read(size_t count){
//     // 内存记录
//     ReadCall* call_ptr = new ReadCall(count);
//     call_seq.push_back(call_ptr);

//     call_ptr->writeToFile(output_filename_bin);


//     // 文件输出
//     char outbuf[1000];
//     writeToMutToolLogFile(output_filename, "read ");
//     sprintf(outbuf, " %ld\n", count);
//     writeToMutToolLogFile(output_filename, outbuf);


//     // 内存记录 · 校验和

//     // 文件输出 · 校验和

// }

void logSysioOfOrinal_write(int fd, const char* buf, size_t count){
    if (openedFileMap_fd2path[fd] == "/dev/null")
        return;
    #ifdef DO_NOT_LOG_AND_CHECK_STDERR
    if (fd == 2)
        return;
    #endif


    long long idx = call_seq.size();

    // 内存记录
    WriteCall* call_ptr = new WriteCall(buf, count);
    call_seq.push_back(call_ptr);


    // 文件输出
    call_ptr->writeToFile(output_filename_bin);


    // char outbuf[1000];
    // writeToMutToolLogFile(output_filename, "write ");
    // writeToMutToolLogFile(output_filename, buf);
    // sprintf(outbuf, " %ld\n", count);
    // writeToMutToolLogFile(output_filename, outbuf);

    std::ostringstream oss;
    oss << "\n{SYSIO_LOG_BEGIN\n"
        << "INDEX: " << idx << "\n"
        << "TYPE: write\n"
        << "TARGET FILE: " << openedFileMap_fd2path[fd] << "\n"
        << "CONTENT: \n";
    writeToMutToolLogFile(output_filename, oss.str().c_str());
    writeToMutToolLogFile(output_filename, call_ptr->getStr(), call_ptr->getCount());

    oss.str("");
    oss << "\n}SYSIO_LOG_END\n";
    writeToMutToolLogFile(output_filename, oss.str().c_str());
    writeToMutToolLogFile("debug1", buf, count);
    // 内存记录 · 校验和

    // 文件输出 · 校验和

}

// void logSysioOfOrinal_lseek(off_t offset, int whence){
//     // 内存记录
//     LseekCall* call_ptr = new LseekCall(offset, whence);
//     call_seq.push_back(call_ptr);

//     call_ptr->writeToFile(output_filename_bin);

//     // 文件输出
//     char outbuf[1000];
//     writeToMutToolLogFile(output_filename, "lseek ");
//     sprintf(outbuf, " %ld %d\n", offset, whence);
//     writeToMutToolLogFile(output_filename, outbuf);

//     // 内存记录 · 校验和

//     // 文件输出 · 校验和

// }


// 子进程中的sisio和原始进程中的进行对比

static unsigned long long cur_sysio_call= 0;


// void checkSysioOfMutant_open(const char* pathname, int flags, int mode){
//     // 直接对比内容
//     OpenCall openCall(pathname, flags, mode);
//     if (!(*(OpenCall*)call_seq[cur_sysio_call++] == openCall)){
//         writeToMutToolLogFile("debug", "killed : ");
//         writeToMutToolLogFile("debug", "ori : ");
//         writeToMutToolLogFile("debug", call_seq[cur_sysio_call-1]->getStr().c_str());
//         writeToMutToolLogFile("debug", " ");
//         writeToMutToolLogFile("debug", "mut : ");
//         writeToMutToolLogFile("debug", openCall.getStr().c_str());
//         writeToMutToolLogFile("debug", "\n");


//         writeToMutToolLogFile("proc_tree", "killed ");
//         exit(42);
//     }
//     // 对比校验和？
// }

// void checkSysioOfMutant_read(size_t count){
//     // 直接对比内容
//     ReadCall readCall(count);
//     if (!(*(ReadCall*)call_seq[cur_sysio_call++] == readCall)){

//         writeToMutToolLogFile("debug", "killed : ");
//         writeToMutToolLogFile("debug", "ori : ");
//         writeToMutToolLogFile("debug", call_seq[cur_sysio_call-1]->getStr().c_str());
//         writeToMutToolLogFile("debug", " ");
//         writeToMutToolLogFile("debug", "mut : ");
//         writeToMutToolLogFile("debug", readCall.getStr().c_str());
//         writeToMutToolLogFile("debug", "\n");
        

//         writeToMutToolLogFile("proc_tree", "killed ");
//         exit(42);
//     }

//     // 对比校验和？
// }

void checkSysioOfMutant_write(int fd, const char* buf, size_t count){
    
    if (openedFileMap_fd2path[fd] == "/dev/null") {
        writeToMutToolLogFile("debug-grep", "write to /dev/null\n");
        return;
    }

    #ifdef DO_NOT_LOG_AND_CHECK_STDERR
    if (fd == 2)
        return;
    #endif

    WriteCall writeCall(buf, count);
    
    // {std::ostringstream oss;
    // oss << "INDEX: " << call_seq[cur_sysio_call]->getStr() << "\n";
    // std::ostringstream logfile;
    // logfile << "log_sysio-" << MUTATION_ID;
    // writeToMutToolLogFile(logfile.str().c_str(), oss.str().c_str());}

    if (MUTATION_ID == 0) {
        // do nothing

        
    }
    else {
        // 直接对比内容
        if (!(*(WriteCall*)call_seq[cur_sysio_call] == writeCall)){

            #ifdef OUTPUT_SYSIO_FOR_DEBUG
            std::ostringstream logfile;
            logfile << "log_sysio-" << MUTATION_ID;

            std::ostringstream oss;
            oss << "\n[KILLED]\n" 
                << "INDEX: " << cur_sysio_call << "\n"
                << "MUTATION_ID: " << MUTATION_ID << "\n" 
                << "TYPE: write\n" 
                << "TARGET FILE: " << openedFileMap_fd2path[fd] << "\n"
                << "MUT CONTENT: \n";
            writeToMutToolLogFile(logfile.str().c_str(), oss.str().c_str());

            writeToMutToolLogFile(logfile.str().c_str(), writeCall.getStr(), writeCall.getCount());

            oss.str("");
            oss << "ORI CONTENT: \n";
            writeToMutToolLogFile(logfile.str().c_str(), oss.str().c_str());

            writeToMutToolLogFile(logfile.str().c_str(), call_seq[cur_sysio_call]->getStr(), call_seq[cur_sysio_call]->getCount());

            oss.str("");
            oss << "\n";
            writeToMutToolLogFile(logfile.str().c_str(), oss.str().c_str());
            #endif

            // writeToMutToolLogFile("proc_tree", "killed \n");
            exit(42);
        }
    }
    

    #ifdef OUTPUT_SYSIO_FOR_DEBUG
    std::ostringstream logfile;
    logfile << "log_sysio-" << MUTATION_ID;

    std::ostringstream oss;
    oss << "\n{SYSIO_LOG_BEGIN\n"
        << "INDEX: " << cur_sysio_call << "\n"
        << "TYPE: write\n"
        << "TARGET FILE: " << openedFileMap_fd2path[fd] << "\n"
        << "CONTENT: \n";
    writeToMutToolLogFile(logfile.str().c_str(), oss.str().c_str());

    writeToMutToolLogFile(logfile.str().c_str(), writeCall.getStr(), writeCall.getCount());

    oss.str("");
    oss << "\n}SYSIO_LOG_END\n";
    writeToMutToolLogFile(logfile.str().c_str(), oss.str().c_str());
    #endif

    ++cur_sysio_call;


    // 对比校验和？
}

// void checkSysioOfMutant_lseek(off_t offset, int whence){
//     // 直接对比内容
//     LseekCall lseekCall(offset, whence);
//     if (!(*(LseekCall*)call_seq[cur_sysio_call++] == lseekCall)){
//         writeToMutToolLogFile("debug", "killed : ");
//         writeToMutToolLogFile("debug", "ori : ");
//         writeToMutToolLogFile("debug", call_seq[cur_sysio_call-1]->getStr().c_str());
//         writeToMutToolLogFile("debug", " ");
//         writeToMutToolLogFile("debug", "mut : ");
//         writeToMutToolLogFile("debug", lseekCall.getStr().c_str());
//         writeToMutToolLogFile("debug", "\n");

//         writeToMutToolLogFile("proc_tree", "killed ");
//         exit(42);
//     }

//     // 对比校验和？
// }




//  ---------------- 输出函数：用于检查 ---------------------

// void log_sysio_call_to_file_for_debug(IOSystemCall* call){
//     std::ostringstream oss;
//     oss << "sysio_seq-" << MUTATION_ID;
//     int fd = __accmut_libc_open(oss.str().c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

//     oss.str("");
//     oss << "\n{\n"
//             << "INDEX: " << cur_sysio_call << "\n"
//             << "TYPE: write\n"
//             << "CONTENT: \n"
//             << call->getStr()
//             << "\n}\n";
//     __accmut_libc_write(fd, (void*)oss.str().c_str(), oss.str().size());
// }



void register_opened_file(int fd, const char* filepath){
    openedFileMap_fd2path[fd] = filepath;
}

void copy_file(const char* sourcePath, const char* destinationPath);

void copy_log_sysio_of_fork_from(int mut_id){
    // std::ostringstream oss;
    // oss << "FORK FROM: " << MUTATION_ID << "\n";
    // std::ostringstream logfile;
    // logfile << "log_sysio_" << mut_id;
    // writeToMutToolLogFile(logfile.str().c_str(), oss.str().c_str());


    std::ostringstream sourcePath, destinationPath;
    const char* prefix = getMutToolLogFilePrefix();
    sourcePath      << prefix << "log_sysio-" << MUTATION_ID;
    destinationPath << prefix << "log_sysio-" << mut_id;
    copy_file(sourcePath.str().c_str(), destinationPath.str().c_str());
}

#define COPY_MUTOUTPUT_FILE_BUFFER_SIZE 4096
    // copy 必定发生在 fork 时
    // input：从父进程继承来的文件描述符
    // return：复制出的文件的文件描述符
void copy_file(const char* sourcePath, const char* destinationPath){
    int sourceFd = __accmut_libc_open(sourcePath,  O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (sourceFd == -1) {
        std::stringstream oss;
        oss << "copy file in accel./sysio: Error opening source file: " << sourcePath << "\n";
        ERROR_MUT_TOOL(oss.str().c_str());
        exit(233);
    }


    int destinationFd;
    char buffer[COPY_MUTOUTPUT_FILE_BUFFER_SIZE];
    ssize_t bytesRead, bytesWritten;
    destinationFd = __accmut_libc_open(destinationPath,  O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (destinationFd == -1) {
        std::stringstream oss;
        oss << "copy file in accel./sysio: Error opening destination file: " << destinationFd << "\n";
        ERROR_MUT_TOOL(oss.str().c_str());
        exit(233);
    }
    // 从源文件读取并写入目标文件
    __accmut_libc_lseek(sourceFd, 0, SEEK_SET); // 从头读取
    while ((bytesRead = __accmut_libc_read(sourceFd, buffer, sizeof(buffer))) > 0) {
        bytesWritten = __accmut_libc_write(destinationFd, buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            std::stringstream oss;
            oss << "copy file in accel./sysio: Error writing to destination file: " << destinationPath << "\n";
            ERROR_MUT_TOOL(oss.str().c_str());
            exit(233);
        }
    }
    __accmut_libc_close(sourceFd);
    __accmut_libc_close(destinationFd);

}
//  ---------------- 输出函数：用于检查 ---------------------



