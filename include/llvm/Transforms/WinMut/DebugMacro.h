// 从 tmpCtrl 文件中读取变异
// #define TMP_CTRL

// 输出proc_tree
// #define PROC_TREE

// 为每个变异构建输出文件(为demo网站)
#define MUT_OUTPUT_FOR_DEMO_SITE

// 为 run 的每一次 case 单独建立输出文件夹，避免覆盖
#define MUT_TOOL

// 为 run 的每一次 case 输出 all_mutation
#define DUMP_ALL_MUTATION

// 记录系统io库调用并对比
#define LOG_SYSIO_CALL

// 从某个case开始执行
// #define CASE_BEGIN_FROM

// 弱变异：影响程序输出的变异
// 通过静态分析提前结束对输出结果不同的进程（不进行fork）
// #define STATIC_ANA_FOR_WEAK_MUTATION


#define NOT_IMPLEMENTED() \
    do { \
        fprintf(stderr, "Error: Function or feature not implemented at %s:%d\n", __FILE__, __LINE__); \
        exit(EXIT_FAILURE); \
    } while(0)

#define ERROR_MUT_TOOL(msg) \
    do { \
        char buf[1000]; \
        sprintf(buf, "%s\n", msg); \
        writeToLogFile("error-mut_tool", getMutToolLogTag()); \
        writeToLogFile("error-mut_tool", buf); \
        writeToMutToolLogFile("error-mut_tool", getMutToolLogTag()); \
        writeToMutToolLogFile("error-mut_tool", buf); \
    } while(0)
