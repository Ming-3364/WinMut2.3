#include "llvm/WinMutRuntime/filesystem/MutOutput.h"
#include <llvm/WinMutRuntime/filesystem/LibCAPI.h>
#include <llvm/WinMutRuntime/mutations/MutationIDDecl.h>
#include <llvm/Transforms/WinMut/DebugMacro.h>
#include <llvm/WinMutRuntime/logging/LogFilePrefix.h>
#include <llvm/WinMutRuntime/logging/LogForMutTool.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sstream>

#include <fstream>

namespace accmut{

    MutOutput *MutOutput::holdptr = nullptr;
    MutOutput *MutOutput::getInstance() {
        if ((holdptr != nullptr)) {
            return holdptr;
        }
        holdptr = new MutOutput();
        return holdptr;
    }
    void MutOutput::hold() { holdptr = getInstance(); }

    // void MutOutput::createStdoutFileFor0(){
    //     createdStdoutFileFor0 = __accmut_libc_open("mut_output-0-stdout", 
    //                                                 O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    //     assert((createdStdoutFileFor0 != -1) && "create stdout file for 0 failed\n");
    // }

    // // TODO: 复制
    // void MutOutput::createStdoutFileForN0(std::vector<int> eq_class_mut_id){
    //     for (auto it = eq_class_mut_id.begin(); it != eq_class_mut_id.end(); ++it){
    //         int mut_id = *it;
    //         assert((mut_id != 0) && "create stdout file for N0 failed : mut_id == 0\n");
    //         if (createdStdoutFileForN0.find(mut_id) == createdStdoutFileForN0.end()){   // stdout file for mut_id not created （may be already cready by its parent process）
    //             char filename[100];
    //             sprintf(filename, "mut_output-%d-stdout", mut_id);

    //             // 为同步在 fork 之前的标准输出，修改为从 createStdoutFileFor0 复制创建
    //             //int fd = __accmut_libc_open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    //             int fd = copy_MutOutputFile(createdStdoutFileFor0, filename);
                

    //             assert((fd != -1) && "create stdout file for N0 failed\n");
    //             assert((createdStdoutFileForN0.find(mut_id) == createdStdoutFileForN0.end()) 
    //                     && "assert stdout file for N0 don't exist failed\n");
    //             createdStdoutFileForN0[mut_id] = fd;
    //         }
            
    //     }
        
    // }

    // bool MutOutput::isStdout(int fd){
    //     return fd == 1;
    // }

    // // TODO: write() return value check
    // void MutOutput::writeStdoutFile(const void *buf, size_t count){
    //     if (isOri()){
    //         __accmut_libc_write(createdStdoutFileFor0, buf, count);
    //     }
    //     else{
    //         for(const auto& createdStdoutFile : createdStdoutFileForN0){
    //             int mut_id = createdStdoutFile.first;
    //             int fd     = createdStdoutFile.second;
    //             assert ((mut_id != 0 && fd != -1) && "write stdout file for N0 : mut_id==0 or fd==-1\n");
    //             __accmut_libc_write(fd, buf, count);
    //         }
    //     }
    // }

    // bool MutOutput::isRegFile(int fd){
    //     struct stat st;
    //     __accmut_libc_fstat(fd, &st);
    //     return S_ISREG(st.st_mode);
    // }

    // 替换为使用 MutToolLogFilePrefix 的版本
    // const char* getMutOutputFilePath(const char* filepath_str, int mut_id, char* buf){
    //     char directory[512];
    //     char filename[512];

    //     const char* lastSlash = strrchr(filepath_str, '/');

    //     if (lastSlash != NULL) {
    //         // 文件路径包含 /
    //         size_t slashIndex = lastSlash - filepath_str + 1;
    //         strncpy(directory, filepath_str, slashIndex);
    //         directory[slashIndex] = '\0';

    //         strcpy(filename, lastSlash + 1);
    //     } else {
    //         // 文件路径没有 /
    //         strcpy(directory, "");
    //         strcpy(filename, filepath_str);
    //     }

    //     const char* mutOutputFilePrefix = "mut_output-";
    //     if (strncmp(filename, mutOutputFilePrefix, strlen(mutOutputFilePrefix)) == 0){ // mut_output-xxx-xxx
    //         const char* secondDash = strchr(strchr(filename, '-') + 1, '-');
    //         strcpy(filename, secondDash + 1);
    //     }

    //     sprintf(buf, "%smut_output-%d-%s", directory, mut_id, filename);
    //     return buf;
    // }

    const char* getMutOutputFilePath(const char* filepath_str, int mut_id, char* buf){
        char filename[2048];
        size_t length = strlen(filepath_str);
        int i = 0;
        for (; i < length; ++i){
            if (filepath_str[i] == '/')
                filename[i] = '_';
            else
                filename[i] = filepath_str[i];
        }
        filename[i] = '\0';

        const char* mutOutputFilePrefix = "mut_output-";
        if (strncmp(filename, mutOutputFilePrefix, strlen(mutOutputFilePrefix)) == 0){ // mut_output-xxx-xxx
            const char* secondDash = strchr(strchr(filename, '-') + 1, '-');
            strcpy(filename, secondDash + 1);
        }

        sprintf(buf, "%smut_output-%d-%s", getMutToolLogFilePrefix(), mut_id, filename);
        return buf;
    }

    int MutOutput::open_stdoutcopy(){
        // 登记 openedFile
        const char* filepath_ori = "stdoutcopy";
        // assert(openedFileSet.find(filepath_ori) == openedFileSet.end() && MUTATION_ID == 0
        //         && "open_stdoutcopy : file reopen\n");
        if (openedFileSet.find(filepath_ori) != openedFileSet.end()){
            std::ostringstream oss;
            oss << "open_stdoutcopy : file reopen : " << filepath_ori << "\n";
            oss << "current openedFileSet:\n";
            int count = 0;
            for (auto it = openedFileSet.begin(); it != openedFileSet.end(); ++it) {
                oss << *it << " ";
                if (++count % 5 == 0) // 每输出5个元素后换行
                    oss << "\n";
            }
            oss << "\n";
            ERROR_MUT_TOOL(oss.str().c_str());
            // ERROR_MUT_TOOL("open_stdoutcopy : file reopen\n");
            // writeToMutToolLogFile("error-mut_tool", "open_stdoutcopy : file reopen\n");
        }

        char buf[1000];
        const char* filepath_mut = getMutOutputFilePath(filepath_ori, MUTATION_ID, buf);
        int fd = __accmut_libc_open(filepath_mut, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (fd == -1) {
            // perror("无法打开文件");
            // exit(EXIT_FAILURE);

            ERROR_MUT_TOOL("open_stdoutcopy: fail to open file\n");
            return -1;
        }
        openedFileMap_fd2path[fd] = filepath_ori;
        openedFileMap_path2fd[filepath_ori] = fd;
        openedFileSet.insert(filepath_ori);
        openedMutOutputFile[MUTATION_ID][filepath_ori] = fd;
        return fd;
    }

    int MutOutput::open_stderrcopy(){
        // 登记 openedFile
        const char* filepath_ori = "stderrcopy";
        // assert(openedFileSet.find(filepath_ori) == openedFileSet.end() && MUTATION_ID == 0
        //         && "open_stderrcopy : file reopen\n");
        if (openedFileSet.find(filepath_ori) != openedFileSet.end()){
            std::ostringstream oss;
            oss << "open_stderrcopy : file reopen : " << filepath_ori << "\n";
            oss << "current openedFileSet:\n";
            int count = 0;
            for (auto it = openedFileSet.begin(); it != openedFileSet.end(); ++it) {
                oss << *it << " ";
                if (++count % 5 == 0) // 每输出5个元素后换行
                    oss << "\n";
            }
            oss << "\n";
            ERROR_MUT_TOOL(oss.str().c_str());
        }

        char buf[1000];
        const char* filepath_mut = getMutOutputFilePath(filepath_ori, MUTATION_ID, buf);
        int fd = __accmut_libc_open(filepath_mut, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (fd == -1) {
            // perror("无法打开文件");
            // exit(EXIT_FAILURE);

            ERROR_MUT_TOOL("open_stderrcopy: fail to open file\n");
            return -1;
        }
        openedFileMap_fd2path[fd] = filepath_ori;
        openedFileMap_path2fd[filepath_ori] = fd;
        openedFileSet.insert(filepath_ori);
        openedMutOutputFile[MUTATION_ID][filepath_ori] = fd;
        return fd;
    }



    void MutOutput::open_and_register_MutOutputFile(const char* filepath_ori, int fd, int flags, int mode){
        if (strcmp(filepath_ori, "/dev/null") == 0) {
            this->fdOfDevNull = fd;
            return;
        }
        // if (flags & O_CREAT){ // 打开的文件是新创建的（不需要考虑读写位置）
        if (1){ // 打开的文件是新创建的（不需要考虑读写位置） 否，可能会引发问题
        
            // if (openedFileSet.find(filepath_ori) != openedFileSet.end()){
            //     // ERROR_MUT_TOOL("open_and_register_MutOutputFile : file reopen\n");
            
            //     std::ostringstream oss;
            //     oss << "open_and_register_MutOutputFile : file reopen : " << filepath_ori << "\n";
            //     oss << "current openedFileSet:\n";
            //     int count = 0;
            //     for (auto it = openedFileSet.begin(); it != openedFileSet.end(); ++it) {
            //         oss << *it << " ";
            //         if (++count % 5 == 0) // 每输出5个元素后换行
            //             oss << "\n";
            //     }
            //     oss << "\n";
            //     ERROR_MUT_TOOL(oss.str().c_str());
            // }

            // openedFileMap_fd2path[fd] = filepath_ori;
            // openedFileMap_path2fd[filepath_ori] = fd;
            // openedFileSet.insert(filepath_ori);

            if (MUTATION_ID == 0){
                if (openedFileSet.find(filepath_ori) == openedFileSet.end()) {
                    char buf[1024];
                    const char* filepath_mut = getMutOutputFilePath(filepath_ori, MUTATION_ID, buf);
                    int MutOutputFile_fd = __accmut_libc_open(filepath_mut, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                    if (MutOutputFile_fd == -1) {
                        // perror("无法打开文件");
                        // exit(EXIT_FAILURE);
                        std::ostringstream oss;
                        oss << "(ORI) open_and_register_MutOutputFile: fail to open file [" << filepath_mut << "]\n";
                        ERROR_MUT_TOOL(oss.str().c_str());
                        return;
                    }
                    
                    openedFileSet.insert(filepath_ori);
                    openedMutOutputFile[MUTATION_ID][filepath_ori] = MutOutputFile_fd;
                }

                openedFileMap_fd2path[fd] = filepath_ori;
                openedFileMap_path2fd[filepath_ori] = fd;
                
            }
            else {  // 在子进程中需要为所有携带的变异创建
                if (openedFileSet.find(filepath_ori) == openedFileSet.end()) {
                    for (int mut_id : eq_class_mut_id){
                        char buf[1024];
                        const char* filepath_mut = getMutOutputFilePath(filepath_ori, mut_id, buf);
                        int MutOutputFile_fd = __accmut_libc_open(filepath_mut, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                        if (MutOutputFile_fd == -1) {
                            // perror("无法打开文件");
                            // exit(EXIT_FAILURE);
                            std::ostringstream oss;
                            oss << "(MUT) open_and_register_MutOutputFile: fail to open file [" << filepath_mut << "]\n";
                            ERROR_MUT_TOOL(oss.str().c_str());
                            return;
                        }
                        openedMutOutputFile[mut_id][filepath_ori] = MutOutputFile_fd;

                    }
                    openedFileSet.insert(filepath_ori);

                }
                
                openedFileMap_fd2path[fd] = filepath_ori;
                openedFileMap_path2fd[filepath_ori] = fd;
            }
            
        }
        else {
            NOT_IMPLEMENTED();
        }
    }

    void MutOutput::prepare_copy(int copy_from, std::vector<int> mut_id_need_remove){
        if (copy_from == 0) {
            this->copy_from = 0;
            return;
        }
        else {
            // 1. 更新 copy_from
            this->copy_from = copy_from;    // 更新 copy_from
            
            // 2. 在fork前剔除需要自立门户的 mut_output（由于单进程，当一个进程自立门户后，它的输出就自己搞定了，不需要再和eq_class一起写）
            for (int mut_id : mut_id_need_remove){
                // auto it = std::find(eq_class_mut_id.begin(), eq_class_mut_id.end(),mut_id);
                // // assert( it != eq_class_mut_id.end() 
                // //     && "prepare_copy : Try removing an ID that doesn't exist\n");
                // if (it == eq_class_mut_id.end()){
                //     std::ostringstream oss;
                //     oss << "prepare_copy : While copy from " << copy_from << ", try to remove a MUTAION_ID that doesn't exist: " << MUTATION_ID << "\n";
                //     oss << "Current eq_class_mud_id = {";
                //     for (int i : eq_class_mut_id) {
                //         oss << i << ", ";
                //     }
                //     oss << "}\n";
                //     ERROR_MUT_TOOL(oss.str().c_str());
                //     return;
                // }
                int flag = 0;
                auto it = eq_class_mut_id.begin();
                for (; it != eq_class_mut_id.end(); ++it){
                    if (*it == mut_id){
                        flag = 1;
                        break;
                    }
                }
                if (flag == 1)
                    eq_class_mut_id.erase(it);
                else {
                    std::ostringstream oss;
                    oss << "prepare_copy : While copy from " << copy_from << ", try to remove a MUTAION_ID that doesn't exist: " << mut_id << "\n";
                    oss << "Current eq_class_mud_id = {";
                    for (int i : eq_class_mut_id) {
                        oss << i << ", ";
                    }
                    oss << "}\n";
                    ERROR_MUT_TOOL(oss.str().c_str());
                    return;
                }
            }
        }
    }

    void MutOutput::copy_and_register_MutOutputFile(std::vector<int> eq_class_mut_id){
        // 重置 eq_class_mut_id
        this->eq_class_mut_id = eq_class_mut_id;
        for (std::string filepath_ori : openedFileSet){
            for (auto it = eq_class_mut_id.begin(); it != eq_class_mut_id.end(); ++it){
                int mut_id = *it;
                // openedMutOutputFile[mut_id][filepath_ori] 有可能已经被复制创建
                if (openedMutOutputFile.find(mut_id) != openedMutOutputFile.end()
                    && openedMutOutputFile[mut_id].find(filepath_ori) != openedMutOutputFile[mut_id].end())
                {
                    continue;        
                }
                char buf[1024];
                const char* filepath_to = getMutOutputFilePath(filepath_ori.c_str(), mut_id, buf);

                int fd = copy_MutOutputFile(openedMutOutputFile[this->copy_from][filepath_ori], filepath_to);

                openedMutOutputFile[mut_id][filepath_ori] = fd;

            }
        }
        // for (auto it = eq_class_mut_id.begin(); it != eq_class_mut_id.end(); ++it){
        //     int mut_id = *it;
        //     assert((mut_id != 0) && "copy_and_register_MutOutputFile failed : mut_id == 0\n");
            
        //     // key_value : filepath_fd
        //     const std::map<int, std::string>& filepath_fd = openedMutOutputFile[MUTATION_ID];
        //     for (const auto& filepath_fd_PAIR : filepath_fd) {
        //         int fd                           = filepath_fd_PAIR.first;
        //         const std::string& filepath_from = filepath_fd_PAIR.second;

        //         char buf[1024];
        //         const char* filepath_to = getMutOutputFilePath(filepath_from.c_str(), mut_id, buf);

        //         copy_MutOutputFile(fd, filepath_to);
                
        //     }
        // }
    }

    void MutOutput::write_registered_MutOutputFile(int fd, const void *buf, size_t count){
        if (fd == this->fdOfDevNull)
            return;

        if (fd == 1)
            fd = createdStdoutFileFor0;
        if (fd == 2)
            fd = createdStderrFileFor0;
        if (openedFileMap_fd2path.find(fd) == openedFileMap_fd2path.end()){
            std::ostringstream oss;
            oss << "write_registered_MutOutputFile: write to unregistered file, we don't kown file name, it's fd is: " << fd << "\n";
            
            char linkname[512];
            ssize_t r;

            // 构造符号链接路径
            snprintf(linkname, sizeof(linkname), "/proc/self/fd/%d", fd);

            // 读取符号链接
            r = __accmut_libc_readlink(linkname, linkname, sizeof(linkname) - 1);
            
            if (r == -1) {
                oss << "readlink error\n";
                return ;
            }
            linkname[r] = '\0'; // 添加字符串结束符

            // 输出文件名
            oss << "File name for fd " << fd << ": " << linkname << std::endl;

            ERROR_MUT_TOOL(oss.str().c_str());
            return;
        }
        const char* filepath_ori = openedFileMap_fd2path[fd].c_str();
        if (MUTATION_ID == 0) {
            if (openedMutOutputFile.find(MUTATION_ID) == openedMutOutputFile.end() 
                || openedMutOutputFile[MUTATION_ID].find(filepath_ori) == openedMutOutputFile[MUTATION_ID].end()) {
                std::ostringstream oss;
                oss << "(ORI) write_registered_MutOutputFile: fail to write file [" << filepath_ori << "] for " << MUTATION_ID <<"\n";
                ERROR_MUT_TOOL(oss.str().c_str());
                return;
            }
            int MutOutputFile_fd = openedMutOutputFile[MUTATION_ID][filepath_ori];
            __accmut_libc_write(MutOutputFile_fd, buf, count);
            // __accmut_libc_write(MutOutputFile_fd, "a write\n", 8);
        }
        else {
            for (auto it = eq_class_mut_id.begin(); it != eq_class_mut_id.end(); ++it){
                int mut_id = *it;

                if (openedMutOutputFile.find(mut_id) == openedMutOutputFile.end() 
                    || openedMutOutputFile[mut_id].find(filepath_ori) == openedMutOutputFile[mut_id].end()) {
                    std::ostringstream oss;
                    oss << "(MUT) write_registered_MutOutputFile: fail to write file [" << filepath_ori << "] for " << mut_id <<"\n";
                    ERROR_MUT_TOOL(oss.str().c_str());
                    if (mut_id < 0) {
                        std::ostringstream oss;
                        oss << "Broken eq_class_mut_id set: "<<"\n";
                        for (auto i : eq_class_mut_id) {
                            oss << i << " ";
                        }
                        oss << "\n";
                        ERROR_MUT_TOOL(oss.str().c_str());
                        exit(233);
                    }
                    return;
                }
                int MutOutputFile_fd = openedMutOutputFile[mut_id][filepath_ori];
                __accmut_libc_write(MutOutputFile_fd, buf, count);
            }
        }
        
    }

    #define COPY_MUTOUTPUT_FILE_BUFFER_SIZE 4096
    // copy 必定发生在 fork 时
    // input：从父进程继承来的文件描述符
    // return：复制出的文件的文件描述符
    int MutOutput::copy_MutOutputFile(int sourceFd, const char* destinationPath){
        off_t oldOffset = __accmut_libc_lseek(sourceFd, 0, SEEK_CUR);   // 保存原offset

        int destinationFd;
        char buffer[COPY_MUTOUTPUT_FILE_BUFFER_SIZE];
        ssize_t bytesRead, bytesWritten;
        destinationFd = __accmut_libc_open(destinationPath, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (destinationFd == -1) {
            // perror("Error opening destination file");
            // close(sourceFd);
            // return EXIT_FAILURE;
            ERROR_MUT_TOOL("copy_MutOutputFile: Error opening destination file\n");
            return -1;
        }
        // 从源文件读取并写入目标文件
        __accmut_libc_lseek(sourceFd, 0, SEEK_SET); // 从头读取
        while ((bytesRead = __accmut_libc_read(sourceFd, buffer, sizeof(buffer))) > 0) {
            bytesWritten = __accmut_libc_write(destinationFd, buffer, bytesRead);
            if (bytesWritten != bytesRead) {
                // perror("Error writing to destination file");
                // close(sourceFd);
                // close(destinationFd);
                // return EXIT_FAILURE;
                ERROR_MUT_TOOL("copy_MutOutputFile: Error writing to destination file\n");
                close(destinationFd);
                return -1;
            }
        }

        __accmut_libc_lseek(sourceFd, oldOffset, SEEK_SET);             // 恢复原offset
        return destinationFd;
    }
}
