# Rest Reminder

Rest Reminder 是一个 Windows 应用程序，帮助您通过定期通知和清除剪贴板来管理工作和休息时间。它在系统托盘中运行，并提供各种功能，如重启 Windows 资源管理器、切换自动启动和清除计时器。

[English](README.md)

## Features 功能

- **System Tray Icon 系统托盘图标**: 应用程序在系统托盘中运行，并提供带有各种选项的上下文菜单。
- **Work Notifications 工作通知**: 每小时通知您休息。
- **Clipboard Clearing 清除剪贴板**: 定期清除剪贴板。
- **Auto Startup 自动启动**: 选项启用或禁用随 Windows 自动启动。
- **Restart Explorer 重启资源管理器**: 选项重启 Windows 资源管理器。
- **Clear Timer 清除计时器**: 选项重置工作计时器。
- **About Dialog 关于对话框**: 显示有关应用程序的信息。

## Installation 安装

1. 克隆仓库:
    ```sh
    git clone https://github.com/hrex39/rest-reminder.git
    cd rest-reminder
    ```

2. 使用提供的构建脚本构建应用程序:
    ```sh
    g++ -o Rest-Reminder Rest-Reminder.cpp -lgdi32 -lcomctl32 -lshell32 -luser32 -mwindows -static
    ```

3. 运行可执行文件:
    ```sh
    ./Rest-Reminder.exe
    ```

## Usage 使用

- **System Tray Menu 系统托盘菜单**:
  - **About 关于**: 显示有关应用程序的信息。
  - **Auto Startup 自动启动**: 切换自动启动功能。
  - **Restart Explorer 重启资源管理器**: 重启 Windows 资源管理器。
  - **Clear Timer 清除计时器**: 重置工作计时器。
  - **Exit 退出**: 退出应用程序。

- **双击**托盘图标清除计时器。

## License 许可证

This project is licensed under the MIT License. See the LICENSE file for details.

此项目采用 MIT 许可证。有关详细信息，请参阅 LICENSE 文件。

## Contributing 贡献

Contributions are welcome! Please open an issue or submit a pull request.

欢迎贡献！请提交问题或拉取请求。

## Author 作者

Huang Chenrui

## Version 版本

20250305 V4.5