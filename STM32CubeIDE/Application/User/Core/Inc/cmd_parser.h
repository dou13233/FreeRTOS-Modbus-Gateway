/*
 * cmd_parser.h
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#ifndef APPLICATION_USER_CORE_INC_CMD_PARSER_H_
#define APPLICATION_USER_CORE_INC_CMD_PARSER_H_
#include "main.h"

// 外部调用接口：传入串口收到的字符串进行解析
void Cmd_Parse(char *cmd_str);


#endif /* APPLICATION_USER_CORE_INC_CMD_PARSER_H_ */
