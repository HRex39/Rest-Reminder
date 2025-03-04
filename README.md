# Rest Reminder

Rest Reminder is a Windows application that helps you manage your work and rest intervals by providing notifications and clearing the clipboard at regular intervals. It runs in the system tray and offers various functionalities such as restarting Windows Explorer, toggling auto startup, and clearing the timer.

[中文](README_zh.md)

## Features

- **System Tray Icon**: The application runs in the system tray and provides a context menu with various options.
- **Work Notifications**: Notifies you every hour to take a break.
- **Clipboard Clearing**: Clears the clipboard at regular intervals.
- **Auto Startup**: Option to enable or disable auto startup with Windows.
- **Restart Explorer**: Option to restart Windows Explorer.
- **Clear Timer**: Option to reset the work timer.
- **About Dialog**: Displays information about the application.

## Installation

1. Clone the repository:
    ```sh
    git clone https://github.com/yourusername/rest-reminder.git
    cd rest-reminder
    ```

2. Build the application using the provided build script:
    ```sh
    g++ -o Rest-Reminder Rest-Reminder.cpp -lgdi32 -lcomctl32 -lshell32 -luser32 -mwindows -static
    ```

3. Run the executable:
    ```sh
    ./Rest-Reminder.exe
    ```

## Usage

- **System Tray Menu**:
  - **About**: Displays information about the application.
  - **Auto Startup**: Toggles the auto startup feature.
  - **Restart Explorer**: Restarts Windows Explorer.
  - **Clear Timer**: Resets the work timer.
  - **Exit**: Exits the application.

- **Double-click** the tray icon to clear the timer.

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request.

## Author

Huang Chenrui

## Version

20250305 V4.5