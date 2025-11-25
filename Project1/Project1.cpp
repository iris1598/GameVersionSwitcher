#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>

// 颜色定义
#define COLOR_BG 0x00F0F0F0
#define COLOR_BTN_BG 0x00E1E1E1
#define COLOR_BTN_HOVER 0x00D0D0D0
#define COLOR_BTN_TEXT 0x00333333
#define COLOR_HEADER 0x00336699
#define COLOR_TEXT 0x00222222

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
    HBRUSH hBgBrush;
    HBRUSH hBtnBrush;

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
        ID_HEADER
    };

public:
    GameVersionSwitcher() : hFont(NULL), hBgBrush(NULL), hBtnBrush(NULL) {
        LoadConfig();

        // 创建字体
        hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "微软雅黑");

        // 创建画刷
        hBgBrush = CreateSolidBrush(COLOR_BG);
        hBtnBrush = CreateSolidBrush(COLOR_BTN_BG);
    }

    ~GameVersionSwitcher() {
        if (hFont) DeleteObject(hFont);
        if (hBgBrush) DeleteObject(hBgBrush);
        if (hBtnBrush) DeleteObject(hBtnBrush);
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

    // 复制目录函数
    bool CopyDirectory(const std::string& from, const std::string& to) {
        // 检查源目录是否存在
        DWORD attr = GetFileAttributesA(from.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            return false;
        }

        std::string searchPath = from + "\\*.*";
        WIN32_FIND_DATAA findFileData;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findFileData);

        if (hFind == INVALID_HANDLE_VALUE) return false;

        // 创建目标目录
        CreateDirectoryA(to.c_str(), NULL);

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
                if (!CopyDirectory(sourcePath, destPath)) {
                    success = FALSE;
                }
            }
            else {
                // 复制文件
                if (!CopyFileA(sourcePath.c_str(), destPath.c_str(), FALSE)) {
                    success = FALSE;
                }
            }
        } while (FindNextFileA(hFind, &findFileData) != 0);

        FindClose(hFind);
        return success;
    }

    // 启动游戏函数
    bool LaunchGame(int version) {
        std::string sourcePath = (version == 1) ? config.version1Path : config.version2Path;

        // 检查路径是否存在
        if (sourcePath.empty() || config.gameFolderPath.empty() || config.gameExePath.empty()) {
            MessageBoxA(hwnd, "路径配置不正确，请通过界面重新配置", "错误", MB_OK | MB_ICONERROR);
            return false;
        }

        // 检查游戏执行文件是否存在
        DWORD attr = GetFileAttributesA(config.gameExePath.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            MessageBoxA(hwnd, "游戏执行文件不存在，请检查路径", "错误", MB_OK | MB_ICONERROR);
            return false;
        }

        // 复制版本文件到游戏目录
        if (!CopyDirectory(sourcePath, config.gameFolderPath)) {
            MessageBoxA(hwnd, "复制文件失败！请检查版本文件夹路径", "错误", MB_OK | MB_ICONERROR);
            return false;
        }

        // 启动游戏
        SHELLEXECUTEINFOA sei = { 0 };
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb = "open";
        sei.lpFile = config.gameExePath.c_str();
        sei.lpDirectory = config.gameFolderPath.c_str(); // 设置工作目录
        sei.nShow = SW_SHOW;

        if (ShellExecuteExA(&sei)) {
            return true;
        }
        else {
            DWORD error = GetLastError();
            std::string errorMsg = "启动游戏失败！错误代码: " + std::to_string(error) +
                "\n游戏路径: " + config.gameExePath;
            MessageBoxA(hwnd, errorMsg.c_str(), "错误", MB_OK | MB_ICONERROR);
            return false;
        }
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
        }
    }

    // 绘制按钮
    void DrawButton(HDC hdc, RECT rect, const char* text, BOOL hover = FALSE) {
        // 绘制按钮背景
        HBRUSH bgBrush = hover ? CreateSolidBrush(COLOR_BTN_HOVER) : hBtnBrush;
        FillRect(hdc, &rect, bgBrush);
        if (hover) DeleteObject(bgBrush);

        // 绘制边框
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
        HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
        SelectObject(hdc, oldPen);
        DeleteObject(borderPen);

        // 绘制文本
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, COLOR_BTN_TEXT);
        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

        DrawTextA(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, oldFont);
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
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, COLOR_TEXT);
            return (LRESULT)hBgBrush;
        }

        case WM_CTLCOLOREDIT:
        {
            HDC hdc = (HDC)wParam;
            SetBkColor(hdc, RGB(255, 255, 255));
            SetTextColor(hdc, COLOR_TEXT);
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        }

        case WM_CTLCOLORBTN:
            return (LRESULT)hBtnBrush;

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
            if (dis->CtlType == ODT_BUTTON) {
                char text[128];
                GetWindowTextA(dis->hwndItem, text, sizeof(text));
                DrawButton(dis->hDC, dis->rcItem, text, dis->itemState & ODS_SELECTED);
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

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    // 创建界面控件
    void CreateControls() {
        int y = 60;  // 为标题栏留出空间
        int labelWidth = 120;
        int editWidth = 350;
        int buttonWidth = 80;
        int height = 28;
        int spacing = 12;

        // 标题
        CreateWindowA("STATIC", "游戏版本切换器", WS_VISIBLE | WS_CHILD | SS_CENTER,
            0, 10, 700, 40, hwnd, (HMENU)ID_HEADER, NULL, NULL);

        // 版本1路径
        CreateWindowA("STATIC", "版本1文件夹:", WS_VISIBLE | WS_CHILD,
            20, y, labelWidth, height, hwnd, NULL, NULL, NULL);
        CreateWindowA("EDIT", config.version1Path.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            20 + labelWidth, y, editWidth, height, hwnd, (HMENU)ID_VERSION1_PATH, NULL, NULL);
        CreateWindowA("BUTTON", "浏览", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            20 + labelWidth + editWidth + 10, y, buttonWidth, height,
            hwnd, (HMENU)ID_BROWSE_VERSION1, NULL, NULL);
        y += height + spacing;

        // 版本2路径
        CreateWindowA("STATIC", "版本2文件夹:", WS_VISIBLE | WS_CHILD,
            20, y, labelWidth, height, hwnd, NULL, NULL, NULL);
        CreateWindowA("EDIT", config.version2Path.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            20 + labelWidth, y, editWidth, height, hwnd, (HMENU)ID_VERSION2_PATH, NULL, NULL);
        CreateWindowA("BUTTON", "浏览", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            20 + labelWidth + editWidth + 10, y, buttonWidth, height,
            hwnd, (HMENU)ID_BROWSE_VERSION2, NULL, NULL);
        y += height + spacing;

        // 游戏执行文件
        CreateWindowA("STATIC", "游戏执行文件:", WS_VISIBLE | WS_CHILD,
            20, y, labelWidth, height, hwnd, NULL, NULL, NULL);
        CreateWindowA("EDIT", config.gameExePath.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            20 + labelWidth, y, editWidth, height, hwnd, (HMENU)ID_GAME_EXE_PATH, NULL, NULL);
        CreateWindowA("BUTTON", "浏览", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            20 + labelWidth + editWidth + 10, y, buttonWidth, height,
            hwnd, (HMENU)ID_BROWSE_GAME_EXE, NULL, NULL);
        y += height + spacing;

        // 游戏文件夹
        CreateWindowA("STATIC", "游戏文件夹:", WS_VISIBLE | WS_CHILD,
            20, y, labelWidth, height, hwnd, NULL, NULL, NULL);
        CreateWindowA("EDIT", config.gameFolderPath.c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            20 + labelWidth, y, editWidth, height, hwnd, (HMENU)ID_GAME_FOLDER_PATH, NULL, NULL);
        CreateWindowA("BUTTON", "浏览", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            20 + labelWidth + editWidth + 10, y, buttonWidth, height,
            hwnd, (HMENU)ID_BROWSE_GAME_FOLDER, NULL, NULL);
        y += height + spacing * 2;

        // 版本1按钮区域
        CreateWindowA("BUTTON", "创建版本1快捷方式", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            20, y, 160, 35, hwnd, (HMENU)ID_CREATE_SHORTCUT1, NULL, NULL);
        CreateWindowA("BUTTON", "测试版本1快捷方式", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            190, y, 160, 35, hwnd, (HMENU)ID_TEST_SHORTCUT1, NULL, NULL);
        CreateWindowA("BUTTON", "启动版本1", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            360, y, 120, 35, hwnd, (HMENU)ID_LAUNCH_VERSION1, NULL, NULL);
        y += 40;

        // 版本2按钮区域
        CreateWindowA("BUTTON", "创建版本2快捷方式", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            20, y, 160, 35, hwnd, (HMENU)ID_CREATE_SHORTCUT2, NULL, NULL);
        CreateWindowA("BUTTON", "测试版本2快捷方式", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            190, y, 160, 35, hwnd, (HMENU)ID_TEST_SHORTCUT2, NULL, NULL);
        CreateWindowA("BUTTON", "启动版本2", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            360, y, 120, 35, hwnd, (HMENU)ID_LAUNCH_VERSION2, NULL, NULL);
        y += 50;

        // 保存配置按钮
        CreateWindowA("BUTTON", "保存配置", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            250, y, 120, 35, hwnd, (HMENU)ID_SAVE_CONFIG, NULL, NULL);

        // 设置所有控件的字体
        EnumChildWindows(hwnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
            SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
            return TRUE;
            }, (LPARAM)hFont);
    }

    // 处理命令消息
    LRESULT HandleCommand(WPARAM wParam, LPARAM lParam) {
        int controlId = LOWORD(wParam);

        switch (controlId) {
        case ID_BROWSE_VERSION1: {
            std::string path = BrowseFolder();
            if (!path.empty()) {
                config.version1Path = path;
                SetDlgItemTextA(hwnd, ID_VERSION1_PATH, path.c_str());
            }
            break;
        }
        case ID_BROWSE_VERSION2: {
            std::string path = BrowseFolder();
            if (!path.empty()) {
                config.version2Path = path;
                SetDlgItemTextA(hwnd, ID_VERSION2_PATH, path.c_str());
            }
            break;
        }
        case ID_BROWSE_GAME_EXE: {
            std::string path = BrowseFile();
            if (!path.empty()) {
                config.gameExePath = path;
                SetDlgItemTextA(hwnd, ID_GAME_EXE_PATH, path.c_str());
            }
            break;
        }
        case ID_BROWSE_GAME_FOLDER: {
            std::string path = BrowseFolder();
            if (!path.empty()) {
                config.gameFolderPath = path;
                SetDlgItemTextA(hwnd, ID_GAME_FOLDER_PATH, path.c_str());
            }
            break;
        }
        case ID_CREATE_SHORTCUT1: {
            std::string desktopPath;
            char path[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, path))) {
                desktopPath = std::string(path) + "\\游戏版本1.lnk";

                if (CreateShortcut(1, desktopPath)) {
                    MessageBoxA(hwnd, "版本1快捷方式创建成功！", "成功", MB_OK);
                }
                else {
                    MessageBoxA(hwnd, "创建快捷方式失败！", "错误", MB_OK | MB_ICONERROR);
                }
            }
            break;
        }
        case ID_CREATE_SHORTCUT2: {
            std::string desktopPath;
            char path[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, path))) {
                desktopPath = std::string(path) + "\\游戏版本2.lnk";

                if (CreateShortcut(2, desktopPath)) {
                    MessageBoxA(hwnd, "版本2快捷方式创建成功！", "成功", MB_OK);
                }
                else {
                    MessageBoxA(hwnd, "创建快捷方式失败！", "错误", MB_OK | MB_ICONERROR);
                }
            }
            break;
        }
        case ID_TEST_SHORTCUT1: {
            // 模拟快捷方式启动
            SetCurrentDirectoryA(GetExeDirectory().c_str());
            if (LaunchGame(1)) {
                MessageBoxA(hwnd, "版本1测试启动成功！", "成功", MB_OK);
            }
            break;
        }
        case ID_TEST_SHORTCUT2: {
            // 模拟快捷方式启动
            SetCurrentDirectoryA(GetExeDirectory().c_str());
            if (LaunchGame(2)) {
                MessageBoxA(hwnd, "版本2测试启动成功！", "成功", MB_OK);
            }
            break;
        }
        case ID_LAUNCH_VERSION1:
            if (LaunchGame(1)) {
                MessageBoxA(hwnd, "版本1启动成功！", "成功", MB_OK);
            }
            break;
        case ID_LAUNCH_VERSION2:
            if (LaunchGame(2)) {
                MessageBoxA(hwnd, "版本2启动成功！", "成功", MB_OK);
            }
            break;
        case ID_SAVE_CONFIG:
            SaveConfig();
            MessageBoxA(hwnd, "配置保存成功！", "成功", MB_OK);
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
            CW_USEDEFAULT, CW_USEDEFAULT, 700, 420, NULL, NULL, GetModuleHandle(NULL), this);

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