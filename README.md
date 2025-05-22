# ComputerSpecificBootTimeVerification 计算机特定的启动时间验证
前提情要：在学校中，总会有很多顽皮的“学生”，早起去玩班里的一体机，为了杜绝此情况，我写了一个 计算机特定的启动时间验证 程序。
它的基本功能：开机自启、未输入正确密码无法关闭弹窗、[默认]7:10分前开机将无法使用（7:10）后自动解锁
默认和直接编译好的exe的密码3180。推荐改掉，去改window.cpp中的const std::wstring PASSWORD = L"3180";就好了
还增加输入密码错误3次直接蓝屏（对电脑没有危害）
*本产品使用C/C++语言编写




根据代码分析，这三个文件构成了一个完整的计算机启动时间验证系统，主要功能如下：
1. main.cpp - 主程序入口：
初始化配置文件
设置开机自启动
记录启动时间
启动其他组件
2. verify.cpp - 验证程序：
监控window.exe运行状态
根据if.json控制程序行为
自动重启window.exe
3. window.cpp - 验证窗口：
全屏密码验证界面
限制系统操作
时间验证功能
错误处理机制

需要特别注意：
各组件间的协作关系
文件路径的一致性
安全机制的实现
ComputerSpecificBootTimeVerificationByRyokuryuneko 系统说明


功能概述：
本系统由三个主要组件构成，用于验证计算机启动时间并实施安全控制。
1. main.exe (主程序)
主要功能：
初始化配置文件（if.json, Standard.json）
添加程序到开机自启动
记录每次启动时间到日志文件
启动verify.exe和配置的软件
配置文件：
%APPDATA%\CSBTVBRN\Time\Standard.json - 存储标准时间
%APPDATA%\CSBTVBRN\Time\YYYY-MM-DD.json - 每日启动日志
%APPDATA%\CSBTVBRN\Start\main.json - 需要自动启动的软件列表
2. verify.exe (验证监控程序)
主要功能：
持续监控window.exe运行状态
根据if.json的value值控制程序行为
自动重启意外关闭的window.exe
当if.json值为"yes"时自动退出
运行机制：
每1秒检查一次系统状态
确保window.exe始终保持运行（除非验证通过）
3. window.exe (验证窗口)
主要功能：
全屏密码验证界面
屏蔽Alt+F4、Ctrl+Esc等系统快捷键
禁用任务管理器
验证当前时间是否符合标准
3次密码错误后执行wb.exe
到达标准时间后自动关闭
安全特性：
窗口置顶无法被覆盖
自动恢复被关闭的窗口
密码验证成功后恢复系统功能
使用说明
1. 首次运行：
main.exe
2. 配置文件：
修改Standard.json设置标准时间
{
    "standard_time": "07:10:00"
}
3. 密码验证：
默认密码：3180
3次错误将触发wb.exe
4. 自动退出：
当系统时间达到Standard.json设置的时间
或输入正确密码后


文件结构：
CSBTVBRN/
├── main.exe
├── verify.exe
├── window.exe
├── wb.exe (可选)
├── libgcc_s_seh-1.dll
├── libstdc++-6.dll
└── libwinpthread-1.dll


注意事项：
1. 需要管理员权限运行
2. 确保所有组件在同一目录
3. 32/64位系统需要对应版本的DLL
4. 修改密码需重新编译window.cpp
