/*
 * cmd_parser.c
 *
 * Created on: 2026年6月1日
 * Author:dd
 */

#include "cmd_parser.h"
#include "storage_service.h" // 引入参数池和保存接口
#include <stdio.h>
#include <string.h>
#include <stdlib.h>          // 🟢 引入 stdlib 库，使用 atoi 函数

/**
 * @brief 解析 ASCII 命令行并执行动作
 * @param cmd_str 以 '\0' 结尾的字符串
 */
//char action[16] = {0};：声明一个 16 字节的字符数组，用来存放指令的“动词”（如 SET/GET）。
//= {0} 是极其优秀的习惯，代表将数组内 16 个字节全部清零（初始化为 \0），防止上一轮的脏数据残留。
//char param[16] 和 val_str[16]：分别存放指令的“名词”（如 TEMP_HIGH）和“数值”（如 "400"）。
//int value = 0;：用来存放最终从 val_str 转换而来的纯数字。
void Cmd_Parse(char *cmd_str)
{
    char action[16] = {0};
    char param[16] = {0};
    char val_str[16] = {0}; // 🟢 重点：把第三个参数也当做纯字符串提取
    int value = 0;

    // 1. 处理无需参数的单指令 (如 SAVE)
    //调用的标准库函数 strncmp
    //参数：cmd_str (目标字符串)，"SAVE" (匹配模板)，4 (最多只比对前 4 个字符)。
    //为什么不用 strcmp？ 因为如果你敲了 SAVE\r\n，带有回车符，strcmp 会认为长度不同而匹配失败。使用 strncmp(..., 4) 只看前 4 个字母，极其鲁棒。
    if (strncmp(cmd_str, "SAVE", 4) == 0) {
        Storage_Save();
        printf("OK\r\n");
    	// printf("OK_TEST_SKIP_SAVE\r\n");
        return;
    }

    // 2. 🟢 核心改动：用全 %s 提取！ %s (String)：代表一串连续的字符串
    //为什么是 %15s 而不是 %16s？
    //代码里定义的数组是 char action[16] = {0};。
    //C 语言的字符串必须以一个看不见的 \0（结束符）结尾。
    // 这样无论你发2个词还是3个词，sscanf 都绝对不会崩溃，而是老老实实返回 2 或 3
    //从一个字符串里，按照指定的格式，把内容提取（抽取）到各个变量中。
    //%s 表示遇到空格就切开
    //15 是保命符！ 它告诉系统，即使外面发来一个 100 字母的长单词，最多也只允许往变量里塞 15 个字母（必须留 1 个位置给 \0 结束符）。这就绝对杜绝了缓冲区溢出（Buffer Overflow）死机！
    //返回值 parsed_count：它会返回成功切出了几个单词。如果你发 GET TEMP，它返回 2；如果你发 SET TEMP 450，它返回 3。
    int parsed_count = sscanf(cmd_str, "%15s %15s %15s", action, param, val_str);

    if (parsed_count >= 2)
    {
        // ============ GET 指令逻辑 (只需要2个参数即可触发) ============
        if (strcmp(action, "GET") == 0)
        {
            if (strcmp(param, "TEMP_HIGH") == 0) {
                printf("VALUE:%d\r\n", g_AppParam.temp_high_limit);
            }
            else if (strcmp(param, "SMOKE_HIGH") == 0) {
                printf("VALUE:%d\r\n", g_AppParam.smoke_high_limit);
            }
            else {
                printf("ERROR\r\n"); // 不认识的参数名
            }
        }
        // ============ SET 指令逻辑 (严格要求必须有3个参数) ============
        else if (strcmp(action, "SET") == 0 && parsed_count == 3)
        {
            // 🟢 在确定有第三个参数后，再安全地把它转换为数字
        	//作用：将纯文本字符串（如 "450"）翻译成 C 语言的整型数字（如 450）。
        	//为什么不直接在上面的 sscanf 里用 %d 提取数字？
        	//如果用户手抖打成了 SET TEMP_HIGH ABC。如果在 sscanf 里用 %d 提取，系统遇到非数字会直接崩溃或返回垃圾值。
        	//先全部按文本 %s 提取出来，确认格式完美无缺后，再单独交给 atoi 处理，这是极其高级的容错架构。即使 val_str 是 "ABC"，atoi 也只会温和地返回 0，而绝对不会引发硬件 HardFault 死机。
            value = atoi(val_str);

            if (strcmp(param, "TEMP_HIGH") == 0) {
                g_AppParam.temp_high_limit = value;
                printf("OK\r\n");
            }
            else if (strcmp(param, "SMOKE_HIGH") == 0) {
                g_AppParam.smoke_high_limit = value;
                printf("OK\r\n");
            }
            else {
                printf("ERROR\r\n");
            }
        }
        else {
            printf("ERROR\r\n"); // 指令格式不对（比如 SET 少了数字，或者乱敲）
        }
    }
    else {
        printf("ERROR\r\n"); // 啥都没解析出来
    }
}
