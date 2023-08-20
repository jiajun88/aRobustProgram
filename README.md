# aRobustProgram
本项目是一个基于TCP协议的多线程文件传输和指令下达的测试程序。

其主要功能包括：
1、使用多线程的方式，可以大批量同时传输多个平台的多个任意不同/相同类型文件；
2、自定义传输协议和指令，实现对平台的基本控制和状态查询；
3、日志记录功能，对每一个平台的相关操作以及其状态都有详细记录，并且实时显示和保存；
4、可以选择数据/文件列表中的任意区间进行下载，并且具有安全检查功能，防止误操作；
5、通过控制控件的可用性，来规范使用者的操作。

文件夹简要说明：
1、Client_fileRead文件夹保存的是负责下载数据/文件的上位机源码；
2、ServerUUV1_fileSend_beifen、ServerUUV2_fileSend_beifen、ServerUUV3_fileSend_beifen分别是
      保存发送数据的模拟下位机的源码。
    【注意】在使用前，你需要先自行选择一个目标文件夹作为待传输的文件夹。

其他说明：
1、项目均在Qt Creator 10.0.1(Community) 、Windows环境下开发。
2、为了方便测试，下位机均为静态IP（实际环境也是），下位机模拟方式为IP地址设为环回地址127.0.0.1，但是各下位机
监听的端口不同，以模拟实际船舱环境IP不同但监听端口相同的实际情况。
3、该测试程序已实际应用（需要根据不同情况根据改进）。

// 作者：jiajunXiong
// 转载请注明出处。
