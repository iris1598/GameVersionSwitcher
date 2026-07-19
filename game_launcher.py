# -*- coding: utf-8 -*-
"""
游戏版本切换器
================
功能：
- 配置两个服务器版本的差异文件路径与游戏目录/exe
- 创建桌面快捷方式，双击即可切换并启动对应版本
- 启动流程：先把指定版本的差异文件覆盖到游戏目录，再启动游戏exe
- 支持命令行 --launch a|b 直接启动对应版本（供快捷方式调用）

配置文件 config.json 与本程序放在同一目录下。
"""

import os
import sys
import json
import time
import shutil
import subprocess
import tkinter as tk
from tkinter import ttk, filedialog, messagebox


CONFIG_FILE = "config.json"


# --------------------------- 配置读写 --------------------------- #

def get_app_dir():
    """获取程序所在目录（兼容 PyInstaller 打包后的 exe）。"""
    if getattr(sys, "frozen", False):
        return os.path.dirname(sys.executable)
    return os.path.dirname(os.path.abspath(__file__))


def get_config_path():
    return os.path.join(get_app_dir(), CONFIG_FILE)


def load_config():
    path = get_config_path()
    if os.path.exists(path):
        try:
            with open(path, "r", encoding="utf-8") as f:
                cfg = json.load(f)
                if isinstance(cfg, dict):
                    return cfg
        except Exception:
            pass
    return {}


def save_config(cfg):
    path = get_config_path()
    with open(path, "w", encoding="utf-8") as f:
        json.dump(cfg, f, ensure_ascii=False, indent=2)


# --------------------------- 文件复制 --------------------------- #

def copy_diff_files(src_dir, dst_dir, log=None):
    """递归地把 src_dir 下的所有文件覆盖到 dst_dir，保持相对目录结构。"""
    if not os.path.isdir(src_dir):
        raise FileNotFoundError(f"差异文件路径不存在：\n{src_dir}")
    if not os.path.isdir(dst_dir):
        os.makedirs(dst_dir, exist_ok=True)

    total = 0
    for root, dirs, files in os.walk(src_dir):
        rel = os.path.relpath(root, src_dir)
        target_dir = dst_dir if rel == "." else os.path.join(dst_dir, rel)
        os.makedirs(target_dir, exist_ok=True)
        for name in files:
            src_file = os.path.join(root, name)
            dst_file = os.path.join(target_dir, name)
            shutil.copy2(src_file, dst_file)
            total += 1
            if log:
                log(f"  复制: {os.path.relpath(dst_file, dst_dir)}")
    return total


# --------------------------- 启动逻辑 --------------------------- #

def launch_version(version_key, log=None):
    """切换并启动指定版本。version_key: 'a' 或 'b'。"""
    cfg = load_config()
    name = cfg.get(f"{version_key}_name", version_key.upper())
    src = cfg.get(f"{version_key}_diff_path", "").strip()
    game_dir = cfg.get("game_path", "").strip()
    game_exe = cfg.get("game_exe", "").strip()

    if not src:
        raise ValueError(f"未设置 {name} 的差异文件路径，请先打开主程序配置。")
    if not game_dir:
        raise ValueError("未设置游戏目录，请先打开主程序配置。")
    if not game_exe:
        raise ValueError("未设置游戏 exe，请先打开主程序配置。")

    if not os.path.isfile(game_exe):
        raise FileNotFoundError(f"游戏 exe 不存在：\n{game_exe}")

    if log:
        log(f"[{time.strftime('%H:%M:%S')}] 正在切换到 {name}...")
        log(f"  源差异文件目录: {src}")
        log(f"  游戏目录: {game_dir}")

    n = copy_diff_files(src, game_dir, log=log)
    if log:
        log(f"  共复制 {n} 个文件")

    if log:
        log(f"[{time.strftime('%H:%M:%S')}] 启动游戏: {game_exe}")

    # 启动游戏（不等待，独立进程）
    subprocess.Popen([game_exe], cwd=os.path.dirname(game_exe), close_fds=True)


# --------------------------- 快捷方式 --------------------------- #

def create_desktop_shortcut(version_key, name):
    """在桌面创建一个指向本程序 --launch <version_key> 的快捷方式。"""
    try:
        import win32com.client
    except ImportError as e:
        raise RuntimeError("缺少 pywin32，无法创建快捷方式。") from e

    shell = win32com.client.Dispatch("WScript.Shell")
    desktop = shell.SpecialFolders("Desktop")

    if getattr(sys, "frozen", False):
        # 打包后的 exe 模式
        target_path = sys.executable
        arguments = f"--launch {version_key}"
    else:
        # 开发模式：用 python.exe 启动脚本
        script_path = os.path.abspath(sys.argv[0])
        target_path = sys.executable  # python.exe
        arguments = f'"{script_path}" --launch {version_key}'

    # 清理桌面同名快捷方式（避免重复）
    safe_name = name.strip() or version_key.upper()
    shortcut_path = os.path.join(desktop, f"{safe_name}.lnk")
    if os.path.exists(shortcut_path):
        os.remove(shortcut_path)

    sc = shell.CreateShortcut(shortcut_path)
    sc.Targetpath = target_path
    sc.Arguments = arguments
    sc.WorkingDirectory = os.path.dirname(target_path)
    sc.Description = f"启动 {safe_name}"
    sc.WindowStyle = 1  # 正常窗口
    sc.save()
    return shortcut_path


# --------------------------- GUI --------------------------- #

class App:
    def __init__(self, root):
        self.root = root
        root.title("游戏版本切换器")
        root.geometry("680x600")
        root.minsize(620, 560)

        cfg = load_config()

        # ---- 版本 A ----
        a_frame = ttk.LabelFrame(root, text="版本 A")
        a_frame.pack(fill="x", padx=10, pady=(10, 4))

        ttk.Label(a_frame, text="名称:").grid(row=0, column=0, sticky="w", padx=5, pady=4)
        self.a_name = ttk.Entry(a_frame, width=20)
        self.a_name.insert(0, cfg.get("a_name", "版本A"))
        self.a_name.grid(row=0, column=1, sticky="w", padx=5, pady=4)

        ttk.Label(a_frame, text="差异文件路径:").grid(row=1, column=0, sticky="w", padx=5, pady=4)
        self.a_path = ttk.Entry(a_frame)
        self.a_path.insert(0, cfg.get("a_diff_path", ""))
        self.a_path.grid(row=1, column=1, columnspan=2, sticky="ew", padx=5, pady=4)
        ttk.Button(a_frame, text="浏览…", width=8, command=lambda: self._browse_dir(self.a_path)).grid(row=1, column=3, padx=5, pady=4)

        # ---- 版本 B ----
        b_frame = ttk.LabelFrame(root, text="版本 B")
        b_frame.pack(fill="x", padx=10, pady=4)

        ttk.Label(b_frame, text="名称:").grid(row=0, column=0, sticky="w", padx=5, pady=4)
        self.b_name = ttk.Entry(b_frame, width=20)
        self.b_name.insert(0, cfg.get("b_name", "版本B"))
        self.b_name.grid(row=0, column=1, sticky="w", padx=5, pady=4)

        ttk.Label(b_frame, text="差异文件路径:").grid(row=1, column=0, sticky="w", padx=5, pady=4)
        self.b_path = ttk.Entry(b_frame)
        self.b_path.insert(0, cfg.get("b_diff_path", ""))
        self.b_path.grid(row=1, column=1, columnspan=2, sticky="ew", padx=5, pady=4)
        ttk.Button(b_frame, text="浏览…", width=8, command=lambda: self._browse_dir(self.b_path)).grid(row=1, column=3, padx=5, pady=4)

        # ---- 游戏设置 ----
        g_frame = ttk.LabelFrame(root, text="游戏")
        g_frame.pack(fill="x", padx=10, pady=4)

        ttk.Label(g_frame, text="游戏目录:").grid(row=0, column=0, sticky="w", padx=5, pady=4)
        self.game_path = ttk.Entry(g_frame)
        self.game_path.insert(0, cfg.get("game_path", ""))
        self.game_path.grid(row=0, column=1, columnspan=2, sticky="ew", padx=5, pady=4)
        ttk.Button(g_frame, text="浏览…", width=8, command=lambda: self._browse_dir(self.game_path)).grid(row=0, column=3, padx=5, pady=4)

        ttk.Label(g_frame, text="游戏 exe:").grid(row=1, column=0, sticky="w", padx=5, pady=4)
        self.game_exe = ttk.Entry(g_frame)
        self.game_exe.insert(0, cfg.get("game_exe", ""))
        self.game_exe.grid(row=1, column=1, columnspan=2, sticky="ew", padx=5, pady=4)
        ttk.Button(g_frame, text="浏览…", width=8, command=self._browse_exe).grid(row=1, column=3, padx=5, pady=4)

        for f in (a_frame, b_frame, g_frame):
            f.columnconfigure(1, weight=1)

        # ---- 操作按钮 ----
        btn_frame = ttk.Frame(root)
        btn_frame.pack(fill="x", padx=10, pady=8)

        ttk.Button(btn_frame, text="💾 保存配置", command=self._save).pack(side="left", padx=4)
        ttk.Separator(btn_frame, orient="vertical").pack(side="left", fill="y", padx=6)
        ttk.Button(btn_frame, text="⭐ 创建A快捷方式", command=lambda: self._create_sc("a")).pack(side="left", padx=4)
        ttk.Button(btn_frame, text="⭐ 创建B快捷方式", command=lambda: self._create_sc("b")).pack(side="left", padx=4)
        ttk.Separator(btn_frame, orient="vertical").pack(side="left", fill="y", padx=6)
        ttk.Button(btn_frame, text="▶ 启动 A", command=lambda: self._launch("a")).pack(side="left", padx=4)
        ttk.Button(btn_frame, text="▶ 启动 B", command=lambda: self._launch("b")).pack(side="left", padx=4)

        # ---- 日志区 ----
        log_frame = ttk.LabelFrame(root, text="日志")
        log_frame.pack(fill="both", expand=True, padx=10, pady=(4, 10))

        self.log = tk.Text(log_frame, height=10, wrap="word", state="disabled",
                           font=("Consolas", 9))
        scroll = ttk.Scrollbar(log_frame, command=self.log.yview)
        self.log.configure(yscrollcommand=scroll.set)
        self.log.pack(side="left", fill="both", expand=True, padx=(5, 0), pady=5)
        scroll.pack(side="right", fill="y", padx=(0, 5), pady=5)

        self._log("欢迎使用游戏版本切换器")
        self._log(f"配置文件位置: {get_config_path()}")

    # -------- 工具方法 --------

    def _browse_dir(self, entry):
        d = filedialog.askdirectory(initialdir=entry.get() or os.path.expanduser("~"))
        if d:
            entry.delete(0, tk.END)
            entry.insert(0, d)

    def _browse_exe(self):
        init = self.game_exe.get() or self.game_path.get() or os.path.expanduser("~")
        f = filedialog.askopenfilename(
            title="选择游戏 exe",
            filetypes=[("可执行文件", "*.exe"), ("所有文件", "*.*")],
            initialdir=os.path.dirname(init) if os.path.exists(init) else init,
        )
        if f:
            self.game_exe.delete(0, tk.END)
            self.game_exe.insert(0, f)
            if not self.game_path.get().strip():
                self.game_path.insert(0, os.path.dirname(f))

    def _log(self, msg):
        ts = time.strftime("%H:%M:%S")
        self.log.configure(state="normal")
        self.log.insert(tk.END, f"[{ts}] {msg}\n")
        self.log.see(tk.END)
        self.log.configure(state="disabled")
        self.root.update_idletasks()

    def _collect(self):
        return {
            "a_name": self.a_name.get().strip() or "版本A",
            "a_diff_path": self.a_path.get().strip(),
            "b_name": self.b_name.get().strip() or "版本B",
            "b_diff_path": self.b_path.get().strip(),
            "game_path": self.game_path.get().strip(),
            "game_exe": self.game_exe.get().strip(),
        }

    # -------- 按钮回调 --------

    def _save(self, silent=False):
        cfg = self._collect()
        save_config(cfg)
        self._log("配置已保存")
        if not silent:
            messagebox.showinfo("提示", "配置已保存")

    def _create_sc(self, key):
        cfg = self._collect()
        save_config(cfg)
        name = cfg.get(f"{key}_name", key.upper())
        try:
            path = create_desktop_shortcut(key, name)
            self._log(f"已创建快捷方式: {path}")
            messagebox.showinfo("成功", f"桌面快捷方式已创建：\n{name}\n\n双击即可启动该版本。")
        except Exception as e:
            self._log(f"创建快捷方式失败: {e}")
            messagebox.showerror("错误", str(e))

    def _launch(self, key):
        cfg = self._collect()
        save_config(cfg)
        name = cfg.get(f"{key}_name", key.upper())
        try:
            launch_version(key, log=self._log)
            self._log(f"{name} 已启动")
            messagebox.showinfo("成功", f"{name} 已启动\n\n差异文件已复制到游戏目录。")
        except Exception as e:
            self._log(f"启动失败: {e}")
            messagebox.showerror("错误", str(e))


# --------------------------- 入口 --------------------------- #

def _cli_launch():
    """命令行模式：--launch a|b，从快捷方式调用。"""
    if len(sys.argv) < 3 or sys.argv[1] != "--launch":
        return False
    key = sys.argv[2].lower()
    if key not in ("a", "b"):
        return False
    try:
        launch_version(key)
    except Exception as e:
        try:
            import ctypes
            ctypes.windll.user32.MessageBoxW(0, str(e), "游戏版本切换器 - 启动失败", 0x10)
        except Exception:
            pass
        sys.exit(1)
    sys.exit(0)


def main():
    # 命令行直接启动模式（被快捷方式调用）
    if _cli_launch():
        return
    # GUI 模式
    root = tk.Tk()
    try:
        # 设置窗口图标（如果有）
        pass
    except Exception:
        pass
    App(root)
    root.mainloop()


if __name__ == "__main__":
    main()
