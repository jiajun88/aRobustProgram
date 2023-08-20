# aRobustProgram
本项目是一个基于TCP协议的多线程文件传输和指令下达的测试程序。

其主要功能包括：
1. 使用多线程的方式，可以同时传输多个来自不同平台的不同/相同类型的文件。
2. 自定义传输协议和指令，实现对平台的基本控制和状态查询。
3. 提供日志记录功能，记录每个平台的操作和状态，实时显示并保存。
4. 支持选择数据/文件列表中的区间进行下载，并具有安全检查功能，防止误操作。
5. 通过控制控件的可用性，规范用户操作。

文件夹概述：
1. "Client_fileRead" 文件夹保存了上位机下载数据/文件的源代码。
2. "ServerUUV1_fileSend_beifen"、"ServerUUV2_fileSend_beifen"、"ServerUUV3_fileSend_beifen" 分别保存了模拟下位机发送数据的源代码。在使用前，您需要自行选择一个目标文件夹作为待传输的文件夹。

其他说明：
1. 本项目在 Windows 环境下使用 Qt Creator 10.0.1 (Community) 进行开发。
2. 为方便测试，下位机均采用静态 IP 地址（实际环境也如此）。模拟时将 IP 地址设置为环回地址 127.0.0.1，但各下位机监听的端口不同，以模拟实际船舱环境中 IP 不同但监听端口相同的情况。
3. 该测试程序已经在实际应用中使用（根据实际情况可能需要进行修改）。

作者：jiajunXiong
转载请注明出处。

# aRobustProgram
This project is a test program based on the TCP protocol for multi-threaded file transmission and command issuing.

Its main features include:
1. Using multi-threading, it can simultaneously transfer multiple arbitrary/different files from different platforms.
2. Customizable transmission protocol and commands, enabling basic control and status queries of platforms.
3. Logging functionality to record detailed operations and statuses of each platform, with real-time display and saving.
4. Selectable data/file ranges for download, with safety checks to prevent accidental operations.
5. Control widget availability to regulate user operations.

Folder Overview:
1. The "Client_fileRead" folder contains the source code for downloading data/files on the upper computer.
2. The "ServerUUV1_fileSend_beifen," "ServerUUV2_fileSend_beifen," and "ServerUUV3_fileSend_beifen" folders contain source codes for simulating sending data on the lower computers. Before using, you need to select a target folder as the source folder for transmission.

Other Notes:
1. The project is developed using Qt Creator 10.0.1 (Community) in a Windows environment.
2. For testing convenience, the lower computers have static IP addresses (which is also the case in the actual environment). The simulation uses the IP address set to the loopback address 127.0.0.1, but each lower computer listens on a different port to simulate the situation where the IP addresses are different but the listening ports are the same.
3. This test program has been used in practical applications (modifications may be necessary according to different situations).

Author: jiajunXiong
Please provide proper credit when reproducing. 



