#include <fcntl.h>
#include <llvm/WinMutRuntime/filesystem/LibCAPI.h>
#include <llvm/WinMutRuntime/filesystem/Path.h>
#include <llvm/WinMutRuntime/logging/LogForMutTool.h>
#include <llvm/WinMutRuntime/logging/LogFilePrefix.h>
#include <llvm/WinMutRuntime/mutations/MutationIDDecl.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <sstream>

using namespace accmut;

// HAVE TO DEFINE THIS TO MAKE TOOL OUTPUT WORK!!!
void mkMutToolLogDir(int cur_case){
    const char* WINMUT_LOG_FILE_PREFIX = getLogFilePrefix();
    char buf[MAXPATHLEN];
    strcpy(buf, WINMUT_LOG_FILE_PREFIX);
    strcat(buf, "run"); // WINMUT_LOG_FILE_PREFIX/run
    if (! directory_exists(buf)) {
        __accmut_libc_mkdir(buf, 0777);
    }

    sprintf(buf, "%s/case_%d/", buf, cur_case); // WINMUT_LOG_FILE_PREFIX/run/case_xx
    __accmut_libc_mkdir(buf, 0777);

    setMutToolLogFilePrefix(buf);
}

// 针对 run 建立的每一个 case 的目录结构，
// 每一次执行 case （新启动一个main函数）时需更新
static char mut_tool_logfile_prefix_for_case[MAXPATHLEN];

void setMutToolLogFilePrefix(const char *input) {
  strcpy(mut_tool_logfile_prefix_for_case, input);
}

const char *getMutToolLogFilePrefix() { return mut_tool_logfile_prefix_for_case; }

std::string getMutToolLogTag() {
  std::ostringstream oss;
  oss << "Tag(" << mut_tool_logfile_prefix_for_case << " " << MUTATION_ID << ")\n";
  return oss.str();
}

void writeToMutToolLogFile(const char *filename, const char *contents, size_t size) {
  char buff[MAXPATHLEN];
  strcpy(buff, mut_tool_logfile_prefix_for_case);
  strcat(buff, filename);
  int fd =
      __accmut_libc_open(buff,  O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  __accmut_libc_write(fd, contents, size);
  __accmut_libc_close(fd);
}

void writeToMutToolLogFile(const char *filename, const char *contents) {
  writeToMutToolLogFile(filename, contents, strlen(contents));
}

void writeToMutToolLogFile(const char *filename, const std::string &contents) {
  writeToMutToolLogFile(filename, contents.c_str(), contents.size());
}


int directory_exists(const char *path) {
    struct stat st;
    if (__accmut_libc_lstat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return 1;  // 目录已存在
    }
    return 0;      // 目录不存在
}
