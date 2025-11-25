#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <map>

// 添加缺失的宏定义
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam) ((int)(short)LOWORD(lParam))
#endif

#ifndef GET_Y_LPARAM  
#define GET_Y_LPARAM(lParam) ((int)(short)HIWORD(lParam))
#endif

// 颜色定义
#define COLOR_BG 0x00F8F9FA
#define COLOR_HEADER_BG 0x00336699
#define COLOR_HEADER_TEXT 0x00FFFFFF
#define COLOR_PANEL_BG 0x00FFFFFF
#define COLOR_PANEL_BORDER 0x00D0D0D0
#define COLOR_BTN_BG 0x004A90E2
#define COLOR_BTN_HOVER 0x003A7BC8
#define COLOR_BTN_ACTIVE 0x002A6BB0
#define COLOR_BTN_TEXT 0x00FFFFFF
#define COLOR_BTN_SECONDARY_BG 0x00E9ECEF
#define COLOR_BTN_SECONDARY_HOVER 0x00D8DADE
#define COLOR_BTN_SECONDARY_ACTIVE 0x00C8CBD0
#define COLOR_BTN_SECONDARY_TEXT 0x00333A44
#define COLOR_TEXT 0x00222A33
#define COLOR_LABEL 0x004A5568
#define COLOR_SUCCESS 0x0028A745
#define COLOR_WARNING 0x00FFC107
#define COLOR_ERROR 0x00DC3545

// 配置文件结构
struct Config {
    std::string version1Path;
    std::string version2Path;
    std::string gameExePath;
    std::string gameFolderPath;
};

class GameVersionSwitcher {
private:
    Config config;
    HWND hwnd;
    HFONT hFont;
    HFONT hTitleFont;
    HFONT hBoldFont;
    HBRUSH hBgBrush;
    HBRUSH hHeaderBrush;
    HBRUSH hPanelBrush;
    HBRUSH hBtnBrush;
    HBRUSH hBtnHoverBrush;
    HBRUSH hBtnActiveBrush;
    HBRUSH hBtnSecondaryBrush;
    HBRUSH hBtnSecondaryHoverBrush;
    HBRUSH hBtnSecondaryActiveBrush;

    int hoverBtnId;
    int activeBtnId;
    bool isCopying;
    int currentProgress;
    HWND hProgressBar;
    HWND hStatusText;
    std::map<int, bool> buttonStates; // 按钮启用状态

    // 控件ID
    enum ControlID {
        ID_VERSION1_PATH = 1000,
        ID_VERSION2_PATH,
        ID_GAME_EXE_PATH,
        ID_GAME_FOLDER_PATH,
        ID_BROWSE_VERSION1,
        ID_BROWSE_VERSION2,
        ID_BROWSE_GAME_EXE,
        ID_BROWSE_GAME_FOLDER,
        ID_CREATE_SHORTCUT1,
        ID_CREATE_SHORTCUT2,
        ID_LAUNCH_VERSION1,
        ID_LAUNCH_VERSION2,
        ID_SAVE_CONFIG,
        ID_TEST_SHORTCUT1,
        ID_TEST_SHORTCUT2,
        ID_HEADER,
        ID_STATUS,
        ID_PROGRESS_BAR
    };

public:
    GameVersionSwitcher() :
        hwnd(NULL),
        hFont(NULL), hTitleFont(NULL), hBoldFont(NULL),
        hBgBrush(NULL), hHeaderBrush(NULL), hPanelBrush(NULL),
        hBtnBrush(NULL), hBtnHoverBrush(NULL), hBtnActiveBrush(NULL),
        hBtnSecondaryBrush(NULL), hBtnSecondaryHoverBrush(NULL), hBtnSecondaryActiveBrush(NULL),
        hoverBtnId(0), activeBtnId(0), isCopying(false), currentProgress(0),
        hProgressBar(NULL), hStatusText(NULL) {

        LoadConfig();

        // 创建字体
        hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        hTitleFont = CreateFont(24, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        hBoldFont = CreateFont(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        // 创建画刷
        hBgBrush = CreateSolidBrush(COLOR_BG);
        hHeaderBrush = CreateSolidBrush(COLOR_HEADER_BG);
        hPanelBrush = CreateSolidBrush(COLOR_PANEL_BG);
        hBtnBrush = CreateSolidBrush(COLOR_BTN_BG);
        hBtnHoverBrush = CreateSolidBrush(COLOR_BTN_HOVER);
        hBtnActiveBrush = CreateSolidBrush(COLOR_BTN_ACTIVE);
        hBtnSecondaryBrush = CreateSolidBrush(COLOR_BTN_SECONDARY_BG);
        hBtnSecondaryHoverBrush = CreateSolidBrush(COLOR_BTN_SECONDARY_HOVER);
        hBtnSecondaryActiveBrush = CreateSolidBrush(COLOR_BTN_SECONDARY_ACTIVE);

        // 初始化所有按钮状态为启用
        InitializeButtonStates();
    }

    ~GameVersionSwitcher() {
        if (hFont) DeleteObject(hFont);
        if (hTitleFont) DeleteObject(hTitleFont);
        if (hBoldFont) DeleteObject(hBoldFont);
        if (hBgBrush) DeleteObject(hBgBrush);
        if (hHeaderBrush) DeleteObject(hHeaderBrush);
        if (hPanelBrush) DeleteObject(hPanelBrush);
        if (hBtnBrush) DeleteObject(hBtnBrush);
        if (hBtnHoverBrush) DeleteObject(hBtnHoverBrush);
        if (hBtnActiveBrush) DeleteObject(hBtnActiveBrush);
        if (hBtnSecondaryBrush) DeleteObject(hBtnSecondaryBrush);
        if (hBtnSecondaryHoverBrush) DeleteObject(hBtnSecondaryHoverBrush);
        if (hBtnSecondaryActiveBrush) DeleteObject(hBtnSecondaryActiveBrush);
    }

    // 初始化按钮状态为启用
    void InitializeButtonStates() {
        int buttonIds[] = {
            ID_BROWSE_VERSION1, ID_BROWSE_VERSION2,
            ID_BROWSE_GAME_EXE, ID_BROWSE_GAME_FOLDER,
            ID_CREATE_SHORTCUT1, ID_CREATE_SHORTCUT2,
            ID_LAUNCH_VERSION1, ID_LAUNCH_VERSION2,
            ID_SAVE_CONFIG, ID_TEST_SHORTCUT1, ID_TEST_SHORTCUT2
        };

        for (int id : buttonIds) {
            buttonStates[id] = true; // 初始状态为启用
        }
    }

    // 获取程序所在目录
    std::string GetExeDirectory() {
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string exeDir = exePath;
        size_t pos = exeDir.find_last_of("\\");
        if (pos != std::string::npos) {
            exeDir = exeDir.substr(0, pos);
        }
        return exeDir;
    }

    // 启用/禁用按钮
    void EnableButtons(bool enable) {
        int buttonIds[] = {
            ID_BROWSE_VERSION1, ID_BROWSE_VERSION2,
            ID_BROWSE_GAME_EXE, ID_BROWSE_GAME_FOLDER,
            ID_CREATE_SHORTCUT1, ID_CREATE_SHORTCUT2,
            ID_LAUNCH_VERSION1, ID_LAUNCH_VERSION2,
            ID_SAVE_CONFIG, ID_TEST_SHORTCUT1, ID_TEST_SHORTCUT2
        };

        for (int id : buttonIds) {
            HWND hButton = GetDlgItem(hwnd, id);
            if (hButton) {
                EnableWindow(hButton, enable);
                buttonStates[id] = enable;
            }
        }

        // 重绘窗口以更新按钮状态
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }

    // 更新进度条
    void UpdateProgress(int progress) {
        currentProgress = progress;
        if (hProgressBar) {
            SendMessage(hProgressBar, PBM_SETPOS, progress, 0);
        }

        // 强制重绘进度条
        if (hProgressBar) {
            InvalidateRect(hProgressBar, NULL, TRUE);
            UpdateWindow(hProgressBar);
        }

        // 处理消息队列，防止界面卡死
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // 设置状态文本
    void SetStatus(const std::string& text, COLORREF color = COLOR_TEXT) {
        if (hStatusText) {
            SetWindowTextA(hStatusText, text.c_str());
            InvalidateRect(hStatusText, NULL, TRUE);
        }

        // 处理消息队列，防止界面卡死
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // 显示错误消息框
    void ShowErrorMessage(const std::string& message) {
        MessageBoxA(hwnd, message.c_str(), "错误", MB_OK | MB_ICONERROR);
    }

    // 检查路径是否有效
    bool IsPathValid(const std::string& path, bool checkDirectory = true) {
        if (path.empty()) {
            return false;
        }

        DWORD attr = GetFileAttributesA(path.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            return false;
        }

        if (checkDirectory) {
            return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
        }
        else {
            return (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
        }
    }

    // 检查文件是否被占用
    bool IsFileInUse(const std::string& filePath) {
        HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ, 0, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            return GetLastError() == ERROR_SHARING_VIOLATION;
        }
        CloseHandle(hFile);
        return false;
    }

    // 安全复制文件
    bool SafeCopyFile(const std::string& from, const std::string& to) {
        for (int i = 0; i < 3; i++) {  // 重试3次
            if (CopyFileA(from.c_str(), to.c_str(), FALSE)) {
                return true;
            }

            if (IsFileInUse(to)) {
                // 目标文件被占用，等待后重试
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            else {
                break;  // 不是占用问题，直接退出
            }
        }
        return false;
    }

    // 复制目录函数
    bool CopyDirectory(const std::string& from, const std::string& to, bool updateProgress = true) {
        // 检查源目录是否存在
        if (!IsPathValid(from, true)) {
            ShowErrorMessage("源目录不存在或无效: " + from);
            return false;
        }

        std::string searchPath = from + "\\*.*";
        WIN32_FIND_DATAA findFileData;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findFileData);

        if (hFind == INVALID_HANDLE_VALUE) {
            ShowErrorMessage("无法访问源目录: " + from);
            return false;
        }

        // 创建目标目录
        if (!CreateDirectoryA(to.c_str(), NULL)) {
            DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS) {
                ShowErrorMessage("无法创建目标目录: " + to);
                FindClose(hFind);
                return false;
            }
        }

        // 先计算文件总数
        int totalFiles = 0;
        int processedFiles = 0;

        do {
            if (strcmp(findFileData.cFileName, ".") == 0 ||
                strcmp(findFileData.cFileName, "..") == 0) {
                continue;
            }
            totalFiles++;
        } while (FindNextFileA(hFind, &findFileData));

        FindClose(hFind);
        hFind = FindFirstFileA(searchPath.c_str(), &findFileData);

        if (hFind == INVALID_HANDLE_VALUE) {
            ShowErrorMessage("无法访问源目录: " + from);
            return false;
        }

        BOOL success = TRUE;
        do {
            if (strcmp(findFileData.cFileName, ".") == 0 ||
                strcmp(findFileData.cFileName, "..") == 0) {
                continue;
            }

            std::string sourcePath = from + "\\" + findFileData.cFileName;
            std::string destPath = to + "\\" + findFileData.cFileName;

            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // 递归复制子目录
                if (!CopyDirectory(sourcePath, destPath, false)) {
                    success = FALSE;
                }
            }
            else {
                // 复制文件
                if (!SafeCopyFile(sourcePath, destPath)) {
                    ShowErrorMessage("复制文件失败: " + sourcePath + " -> " + destPath);
                    success = FALSE;
                }
            }

            // 更新进度
            if (updateProgress) {
                processedFiles++;
                int progress = (processedFiles * 100) / totalFiles;
                UpdateProgress(progress);

                // 短暂延迟，避免切换过快
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        } while (FindNextFileA(hFind, &findFileData) != 0);

        FindClose(hFind);
        return success;
    }

    // 验证所有必要的路径
    bool ValidatePaths(int version) {
        std::string sourcePath = (version == 1) ? config.version1Path : config.version2Path;

        // 检查源路径
        if (sourcePath.empty()) {
            ShowErrorMessage("版本" + std::to_string(version) + "路径未设置");
            return false;
        }

        if (!IsPathValid(sourcePath, true)) {
            ShowErrorMessage("版本" + std::to_string(version) + "路径无效: " + sourcePath);
            return false;
        }

        // 检查游戏执行文件
        if (config.gameExePath.empty()) {
            ShowErrorMessage("游戏执行文件路径未设置");
            return false;
        }

        if (!IsPathValid(config.gameExePath, false)) {
            ShowErrorMessage("游戏执行文件不存在: " + config.gameExePath);
            return false;
        }

        // 检查游戏文件夹
        if (config.gameFolderPath.empty()) {
            ShowErrorMessage("游戏文件夹路径未设置");
            return false;
        }

        if (!IsPathValid(config.gameFolderPath, true)) {
            ShowErrorMessage("游戏文件夹路径无效: " + config.gameFolderPath);
            return false;
        }

        return true;
    }

    // 启动游戏函数
    bool LaunchGame(int version) {
        if (isCopying) {
            MessageBoxA(hwnd, "正在复制文件，请稍候...", "提示", MB_OK | MB_ICONINFORMATION);
            return false;
        }

        // 验证所有路径
        if (!ValidatePaths(version)) {
            return false;
        }

        std::string sourcePath = (version == 1) ? config.version1Path : config.version2Path;

        // 在新线程中执行复制和启动操作
        std::thread([this, version, sourcePath]() {
            isCopying = true;
            EnableButtons(false);

            SetStatus("正在准备文件...");
            UpdateProgress(0);

            // 复制版本文件到游戏目录
            SetStatus("正在复制文件...");
            if (!CopyDirectory(sourcePath, config.gameFolderPath)) {
                SetStatus("错误：复制文件失败");
                isCopying = false;
                EnableButtons(true);
                UpdateProgress(0);
                return;
            }

            SetStatus("正在启动游戏...");
            UpdateProgress(100);

            // 启动游戏
            SHELLEXECUTEINFOA sei = { 0 };
            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_NOCLOSEPROCESS;
            sei.lpVerb = "open";
            sei.lpFile = config.gameExePath.c_str();
            sei.lpDirectory = config.gameFolderPath.c_str();
            sei.nShow = SW_SHOW;

            if (ShellExecuteExA(&sei)) {
                SetStatus("游戏启动成功！");
            }
            else {
                DWORD error = GetLastError();
                std::string errorMsg = "启动失败！错误代码: " + std::to_string(error);
                SetStatus(errorMsg);
                ShowErrorMessage("无法启动游戏: " + config.gameExePath + "\n错误代码: " + std::to_string(error));
            }

            // 短暂延迟后重置状态
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            isCopying = false;
            EnableButtons(true);
            UpdateProgress(0);
            SetStatus("就绪");

            }).detach();

        return true;
    }

    // 创建快捷方式
    bool CreateShortcut(int version, const std::string& shortcutPath) {
        CoInitialize(NULL);

        IShellLinkA* pShellLink;
        HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
            IID_IShellLinkA, (LPVOID*)&pShellLink);

        if (SUCCEEDED(hr)) {
            // 获取当前程序路径
            char exePath[MAX_PATH];
            GetModuleFileNameA(NULL, exePath, MAX_PATH);

            // 设置快捷方式参数
            std::string parameters = std::to_string(version);
            pShellLink->SetPath(exePath);
            pShellLink->SetArguments(parameters.c_str());

            // 设置工作目录
            std::string workingDir = GetExeDirectory();
            pShellLink->SetWorkingDirectory(workingDir.c_str());

            std::string description = (version == 1) ? "游戏版本1" : "游戏版本2";
            pShellLink->SetDescription(description.c_str());

            // 保存快捷方式
            IPersistFile* pPersistFile;
            hr = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);

            if (SUCCEEDED(hr)) {
                WCHAR wszShortcutPath[MAX_PATH];
                MultiByteToWideChar(CP_ACP, 0, shortcutPath.c_str(), -1,
                    wszShortcutPath, MAX_PATH);

                hr = pPersistFile->Save(wszShortcutPath, TRUE);
                pPersistFile->Release();
            }

            pShellLink->Release();
        }

        CoUninitialize();
        return SUCCEEDED(hr);
    }

    // 浏览文件夹对话框
    std::string BrowseFolder() {
        BROWSEINFOA bi = { 0 };
        bi.lpszTitle = "选择文件夹";
        bi.hwndOwner = hwnd;
        LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);

        if (pidl != NULL) {
            char path[MAX_PATH];
            SHGetPathFromIDListA(pidl, path);
            CoTaskMemFree(pidl);
            return std::string(path);
        }
        return "";
    }

    // 浏览文件对话框
    std::string BrowseFile(const char* filter = "可执行文件 (*.exe)\0*.exe\0所有文件 (*.*)\0*.*\0") {
        OPENFILENAMEA ofn;
        char filePath[MAX_PATH] = { 0 };

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = filePath;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileNameA(&ofn)) {
            return std::string(filePath);
        }
        return "";
    }

    // 加载配置
    void LoadConfig() {
        std::string configFile = GetExeDirectory() + "\\config.ini";
        std::ifstream file(configFile);
        if (file.is_open()) {
            std::getline(file, config.version1Path);
            std::getline(file, config.version2Path);
            std::getline(file, config.gameExePath);
            std::getline(file, config.gameFolderPath);
            file.close();
        }
    }

    // 保存配置
    void SaveConfig() {
        std::string configFile = GetExeDirectory() + "\\config.ini";
        std::ofstream file(configFile);
        if (file.is_open()) {
            file << config.version1Path << std::endl;
            file << config.version2Path << std::endl;
            file << config.gameExePath << std::endl;
            file << config.gameFolderPath << std::endl;
            file.close();
            SetStatus("配置保存成功！");
        }
        else {
            SetStatus("错误：保存配置失败");
            ShowErrorMessage("无法保存配置文件: " + configFile);
        }
    }

    // 绘制圆角矩形
    void DrawRoundRect(HDC hdc, RECT rect, int radius, HBRUSH brush) {
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

        RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);

        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(pen);
    }

    // 绘制按钮
    void DrawButton(HDC hdc, HWND hButton, const char* text, BOOL hover = FALSE, BOOL active = FALSE, BOOL primary = TRUE) {
        RECT rect;
        GetClientRect(hButton, &rect);

        int ctrlId = GetDlgCtrlID(hButton);
        bool enabled = buttonStates[ctrlId];

        HBRUSH bgBrush;
        COLORREF textColor;

        if (!enabled) {
            // 禁用状态
            bgBrush = CreateSolidBrush(RGB(240, 240, 240));
            textColor = RGB(180, 180, 180);
        }
        else if (primary) {
            if (active) {
                bgBrush = hBtnActiveBrush;
            }
            else if (hover) {
                bgBrush = hBtnHoverBrush;
            }
            else {
                bgBrush = hBtnBrush;
            }
            textColor = COLOR_BTN_TEXT;
        }
        else {
            if (active) {
                bgBrush = hBtnSecondaryActiveBrush;
            }
            else if (hover) {
                bgBrush = hBtnSecondaryHoverBrush;
            }
            else {
                bgBrush = hBtnSecondaryBrush;
            }
            textColor = COLOR_BTN_SECONDARY_TEXT;
        }

        // 绘制按钮背景
        DrawRoundRect(hdc, rect, 6, bgBrush);

        // 绘制文本
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, textColor);
        HFONT oldFont = (HFONT)SelectObject(hdc, hBoldFont);

        DrawTextA(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, oldFont);

        if (!enabled) {
            DeleteObject(bgBrush);
        }
    }

    // 窗口过程函数
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        GameVersionSwitcher* pThis = nullptr;

        if (msg == WM_NCCREATE) {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = reinterpret_cast<GameVersionSwitcher*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
            pThis->hwnd = hwnd;
        }
        else {
            pThis = reinterpret_cast<GameVersionSwitcher*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (pThis) {
            return pThis->HandleMessage(msg, wParam, lParam);
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE:
            CreateControls();
            return 0;

        case WM_COMMAND:
            return HandleCommand(wParam, lParam);

        case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;
            HWND hwndCtrl = (HWND)lParam;
            int ctrlId = GetDlgCtrlID(hwndCtrl);

            if (ctrlId == ID_HEADER) {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, COLOR_HEADER_TEXT);
                return (LRESULT)hHeaderBrush;
            }
            else if (ctrlId == ID_STATUS) {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, COLOR_TEXT);
                return (LRESULT)hBgBrush;
            }
            else {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, COLOR_LABEL);
                return (LRESULT)hBgBrush;
            }
        }

        case WM_CTLCOLOREDIT:
        {
            HDC hdc = (HDC)wParam;
            SetBkColor(hdc, COLOR_PANEL_BG);
            SetTextColor(hdc, COLOR_TEXT);
            return (LRESULT)hPanelBrush;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
            if (dis->CtlType == ODT_BUTTON) {
                char text[128];
                GetWindowTextA(dis->hwndItem, text, sizeof(text));

                BOOL isPrimary = (dis->CtlID == ID_LAUNCH_VERSION1 || dis->CtlID == ID_LAUNCH_VERSION2);
                BOOL hover = (dis->itemState & ODS_SELECTED) || (hoverBtnId == dis->CtlID);
                BOOL active = (activeBtnId == dis->CtlID);

                DrawButton(dis->hDC, dis->hwndItem, text, hover, active, isPrimary);
                return TRUE;
            }
            break;
        }

        case WM_ERASEBKGND:
        {
            RECT rect;
            GetClientRect(hwnd, &rect);
            FillRect((HDC)wParam, &rect, hBgBrush);
            return 1;
        }

        case WM_MOUSEMOVE:
        {
            // 使用我们定义的宏获取鼠标坐标
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);

            // 检测鼠标在哪个按钮上
            int oldHoverBtnId = hoverBtnId;
            hoverBtnId = 0;

            HWND hChild = GetWindow(hwnd, GW_CHILD);
            while (hChild) {
                int ctrlId = GetDlgCtrlID(hChild);
                if (ctrlId >= ID_BROWSE_VERSION1 && ctrlId <= ID_SAVE_CONFIG) {
                    RECT rect;
                    GetWindowRect(hChild, &rect);
                    POINT pt = { xPos, yPos };
                    ClientToScreen(hwnd, &pt);

                    if (PtInRect(&rect, pt)) {
                        hoverBtnId = ctrlId;
                        break;
                    }
                }
                hChild = GetNextWindow(hChild, GW_HWNDNEXT);
            }

            if (hoverBtnId != oldHoverBtnId) {
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        }

        case WM_LBUTTONDOWN:
        {
            // 使用我们定义的宏获取鼠标坐标
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);

            HWND hChild = GetWindow(hwnd, GW_CHILD);
            while (hChild) {
                int ctrlId = GetDlgCtrlID(hChild);
                if (ctrlId >= ID_BROWSE_VERSION1 && ctrlId <= ID_SAVE_CONFIG) {
                    RECT rect;
                    GetWindowRect(hChild, &rect);
                    POINT pt = { xPos, yPos };
                    ClientToScreen(hwnd, &pt);

                    if (PtInRect(&rect, pt) && buttonStates[ctrlId]) {
                        activeBtnId = ctrlId;
                        InvalidateRect(hChild, NULL, TRUE);
                        break;
                    }
                }
                hChild = GetNextWindow(hChild, GW_HWNDNEXT);
            }
            break;
        }

        case WM_LBUTTONUP:
        {
            if (activeBtnId != 0) {
                InvalidateRect(GetDlgItem(hwnd, activeBtnId), NULL, TRUE);
                activeBtnId = 0;
            }
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    // 创建界面控件
    void CreateControls() {
        int y = 80;  // 为标题栏留出空间
        int labelWidth = 120;
        int editWidth = 380;
        int buttonWidth = 80;
        int height = 32;
        int spacing = 15;

        // 标题
        CreateWindowA("STATIC", "游戏版本切换器", WS_VISIBLE | WS_CHILD | SS_CENTER,
            0, 20, 700, 40, hwnd, (HMENU)ID_HEADER, NULL, NULL);

        // 版本1路径
        CreateWindowA("STATIC", "版本1文件夹:", WS_VISIBLE | WS_CHILD,
            30, y, labelWidth, height, hwnd, NULL, NULL, NULL);
        CreateWindowA("EDIT", config.version1Path.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            30 + labelWidth, y, editWidth, height, hwnd, (HMENU)ID_VERSION1_PATH, NULL, NULL);
        CreateWindowA("BUTTON", "浏览", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            30 + labelWidth + editWidth + 10, y, buttonWidth, height,
            hwnd, (HMENU)ID_BROWSE_VERSION1, NULL, NULL);
        y += height + spacing;

        // 版本2路径
        CreateWindowA("STATIC", "版本2文件夹:", WS_VISIBLE | WS_CHILD,
            30, y, labelWidth, height, hwnd, NULL, NULL, NULL);
        CreateWindowA("EDIT", config.version2Path.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            30 + labelWidth, y, editWidth, height, hwnd, (HMENU)ID_VERSION2_PATH, NULL, NULL);
        CreateWindowA("BUTTON", "浏览", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            30 + labelWidth + editWidth + 10, y, buttonWidth, height,
            hwnd, (HMENU)ID_BROWSE_VERSION2, NULL, NULL);
        y += height + spacing;

        // 游戏执行文件
        CreateWindowA("STATIC", "游戏执行文件:", WS_VISIBLE | WS_CHILD,
            30, y, labelWidth, height, hwnd, NULL, NULL, NULL);
        CreateWindowA("EDIT", config.gameExePath.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            30 + labelWidth, y, editWidth, height, hwnd, (HMENU)ID_GAME_EXE_PATH, NULL, NULL);
        CreateWindowA("BUTTON", "浏览", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            30 + labelWidth + editWidth + 10, y, buttonWidth, height,
            hwnd, (HMENU)ID_BROWSE_GAME_EXE, NULL, NULL);
        y += height + spacing;

        // 游戏文件夹
        CreateWindowA("STATIC", "游戏文件夹:", WS_VISIBLE | WS_CHILD,
            30, y, labelWidth, height, hwnd, NULL, NULL, NULL);
        CreateWindowA("EDIT", config.gameFolderPath.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            30 + labelWidth, y, editWidth, height, hwnd, (HMENU)ID_GAME_FOLDER_PATH, NULL, NULL);
        CreateWindowA("BUTTON", "浏览", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            30 + labelWidth + editWidth + 10, y, buttonWidth, height,
            hwnd, (HMENU)ID_BROWSE_GAME_FOLDER, NULL, NULL);
        y += height + spacing * 2;

        // 版本1按钮区域
        CreateWindowA("BUTTON", "创建版本1快捷方式", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            30, y, 160, 38, hwnd, (HMENU)ID_CREATE_SHORTCUT1, NULL, NULL);
        CreateWindowA("BUTTON", "测试版本1快捷方式", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            200, y, 160, 38, hwnd, (HMENU)ID_TEST_SHORTCUT1, NULL, NULL);
        CreateWindowA("BUTTON", "启动版本1", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            370, y, 120, 38, hwnd, (HMENU)ID_LAUNCH_VERSION1, NULL, NULL);
        y += 45;

        // 版本2按钮区域
        CreateWindowA("BUTTON", "创建版本2快捷方式", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            30, y, 160, 38, hwnd, (HMENU)ID_CREATE_SHORTCUT2, NULL, NULL);
        CreateWindowA("BUTTON", "测试版本2快捷方式", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            200, y, 160, 38, hwnd, (HMENU)ID_TEST_SHORTCUT2, NULL, NULL);
        CreateWindowA("BUTTON", "启动版本2", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            370, y, 120, 38, hwnd, (HMENU)ID_LAUNCH_VERSION2, NULL, NULL);
        y += 50;

        // 保存配置按钮
        CreateWindowA("BUTTON", "保存配置", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            280, y, 120, 38, hwnd, (HMENU)ID_SAVE_CONFIG, NULL, NULL);
        y += 50;

        // 进度条
        CreateWindowA("STATIC", "进度:", WS_VISIBLE | WS_CHILD,
            30, y, 40, 20, hwnd, NULL, NULL, NULL);

        hProgressBar = CreateWindowExA(0, PROGRESS_CLASSA, NULL,
            WS_VISIBLE | WS_CHILD,
            80, y, 500, 20, hwnd, (HMENU)ID_PROGRESS_BAR, NULL, NULL);

        SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessage(hProgressBar, PBM_SETSTEP, 1, 0);
        SendMessage(hProgressBar, PBM_SETPOS, 0, 0);

        y += 30;

        // 状态文本
        hStatusText = CreateWindowA("STATIC", "就绪", WS_VISIBLE | WS_CHILD,
            30, y, 640, 25, hwnd, (HMENU)ID_STATUS, NULL, NULL);

        // 设置字体
        SendDlgItemMessage(hwnd, ID_HEADER, WM_SETFONT, (WPARAM)hTitleFont, TRUE);

        EnumChildWindows(hwnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
            int ctrlId = GetDlgCtrlID(hwnd);
            if (ctrlId != ID_HEADER && ctrlId != ID_PROGRESS_BAR) {
                SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
            }
            return TRUE;
            }, (LPARAM)hFont);
    }

    // 处理命令消息
    LRESULT HandleCommand(WPARAM wParam, LPARAM lParam) {
        int controlId = LOWORD(wParam);

        // 检查按钮是否被禁用
        if (buttonStates.find(controlId) != buttonStates.end() && !buttonStates[controlId]) {
            return 0; // 按钮被禁用，不处理点击
        }

        switch (controlId) {
        case ID_BROWSE_VERSION1: {
            std::string path = BrowseFolder();
            if (!path.empty()) {
                config.version1Path = path;
                SetDlgItemTextA(hwnd, ID_VERSION1_PATH, path.c_str());
                SetStatus("版本1路径已更新");
            }
            break;
        }
        case ID_BROWSE_VERSION2: {
            std::string path = BrowseFolder();
            if (!path.empty()) {
                config.version2Path = path;
                SetDlgItemTextA(hwnd, ID_VERSION2_PATH, path.c_str());
                SetStatus("版本2路径已更新");
            }
            break;
        }
        case ID_BROWSE_GAME_EXE: {
            std::string path = BrowseFile();
            if (!path.empty()) {
                config.gameExePath = path;
                SetDlgItemTextA(hwnd, ID_GAME_EXE_PATH, path.c_str());
                SetStatus("游戏执行文件路径已更新");
            }
            break;
        }
        case ID_BROWSE_GAME_FOLDER: {
            std::string path = BrowseFolder();
            if (!path.empty()) {
                config.gameFolderPath = path;
                SetDlgItemTextA(hwnd, ID_GAME_FOLDER_PATH, path.c_str());
                SetStatus("游戏文件夹路径已更新");
            }
            break;
        }
        case ID_CREATE_SHORTCUT1: {
            std::string desktopPath;
            char path[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, path))) {
                desktopPath = std::string(path) + "\\游戏版本1.lnk";

                if (CreateShortcut(1, desktopPath)) {
                    SetStatus("版本1快捷方式创建成功！");
                }
                else {
                    SetStatus("创建快捷方式失败！");
                    ShowErrorMessage("无法创建快捷方式: " + desktopPath);
                }
            }
            else {
                ShowErrorMessage("无法获取桌面路径");
            }
            break;
        }
        case ID_CREATE_SHORTCUT2: {
            std::string desktopPath;
            char path[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, path))) {
                desktopPath = std::string(path) + "\\游戏版本2.lnk";

                if (CreateShortcut(2, desktopPath)) {
                    SetStatus("版本2快捷方式创建成功！");
                }
                else {
                    SetStatus("创建快捷方式失败！");
                    ShowErrorMessage("无法创建快捷方式: " + desktopPath);
                }
            }
            else {
                ShowErrorMessage("无法获取桌面路径");
            }
            break;
        }
        case ID_TEST_SHORTCUT1: {
            // 模拟快捷方式启动
            SetCurrentDirectoryA(GetExeDirectory().c_str());
            std::thread([this]() {
                if (LaunchGame(1)) {
                    // 成功消息已经在LaunchGame中设置
                }
                }).detach();
            break;
        }
        case ID_TEST_SHORTCUT2: {
            // 模拟快捷方式启动
            SetCurrentDirectoryA(GetExeDirectory().c_str());
            std::thread([this]() {
                if (LaunchGame(2)) {
                    // 成功消息已经在LaunchGame中设置
                }
                }).detach();
            break;
        }
        case ID_LAUNCH_VERSION1:
            SetCurrentDirectoryA(GetExeDirectory().c_str());
            std::thread([this]() {
                LaunchGame(1);
                }).detach();
            break;
        case ID_LAUNCH_VERSION2:
            SetCurrentDirectoryA(GetExeDirectory().c_str());
            std::thread([this]() {
                LaunchGame(2);
                }).detach();
            break;
        case ID_SAVE_CONFIG:
            SaveConfig();
            break;
        }
        return 0;
    }

    // 运行程序
    int Run() {
        // 检查命令行参数
        int argc;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

        if (argc > 1) {
            // 命令行模式，直接启动指定版本
            int version = _wtoi(argv[1]);

            // 设置当前目录为程序所在目录
            std::string exeDir = GetExeDirectory();
            SetCurrentDirectoryA(exeDir.c_str());

            LoadConfig();

            // 验证路径
            if (!ValidatePaths(version)) {
                LocalFree(argv);
                return 1;
            }

            if (LaunchGame(version)) {
                LocalFree(argv);
                return 0;
            }
            else {
                LocalFree(argv);
                return 1;
            }
        }

        LocalFree(argv);

        // GUI模式
        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "GameVersionSwitcher";
        wc.hbrBackground = hBgBrush;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.style = CS_HREDRAW | CS_VREDRAW;

        RegisterClassA(&wc);

        // 创建窗口
        hwnd = CreateWindowA("GameVersionSwitcher", "游戏版本切换器",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, 700, 500, NULL, NULL, GetModuleHandle(NULL), this);

        // 居中显示窗口
        RECT rc;
        GetWindowRect(hwnd, &rc);
        int xPos = (GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2;
        int yPos = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2;
        SetWindowPos(hwnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return (int)msg.wParam;
    }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    GameVersionSwitcher app;
    return app.Run();
}