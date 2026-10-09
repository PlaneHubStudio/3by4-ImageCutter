# 3:4 图片快切

PlaneHub 的轻量桌面裁图工具。拖入一张长图，自动切成两张或三张 **3:4** 图片，用于连续翻页发布。

- 3:2 原图切成两张，9:4 原图切成三张。
- 其他比例比较两种方案的保留面积，选择最少裁剪方案并居中裁剪。
- 为严格满足整数像素的 3:4，边缘可能额外裁掉少量像素；不拉伸、不放大。
- 输出 PNG，按 `01`、`02`、`03` 从左到右编号；每次新建输出文件夹，保留原图。
- 切片展开动画、按钮悬停反馈、PlaneHub Logo 拆分动画和飞机重叠卡片图标。
- 完全本地处理，不连接网络。

## 下载当前版本

| 平台 | 下载 | 支持 |
| --- | --- | --- |
| macOS | [DMG](https://github.com/PlaneHubStudio/3by4-ImageCutter/raw/refs/heads/main/releases/v1.3.0/ImageCutter-macOS.dmg) | macOS 11+，Apple Silicon / Intel 通用 |
| Windows | [EXE](https://github.com/PlaneHubStudio/3by4-ImageCutter/raw/refs/heads/main/releases/v1.3.0/ImageCutter-Windows-x64.exe) | Windows 10 / 11，x64，免安装 |

macOS 支持 JPG / PNG / HEIC / TIFF / BMP；Windows 支持 JPG / PNG / TIFF / BMP，暂不支持 HEIC。

Mac：打开 DMG，把 App 拖进 Applications；更新前先退出旧版并替换安装。
Windows：下载 EXE 后直接打开。

当前 Mac App 使用本机临时签名，未进行 Apple 开发者签名或公证；Windows EXE 未进行发布者签名。Windows 版已完成交叉编译、裁剪算法和 PE 结构检查，**尚未在真实 Windows 上运行验证**。

## 源码

- `Sources/main.swift`：macOS 原生 AppKit 界面、图片裁剪和导出。
- `Sources/PlaneHubLogo.swift`：从原始 SVG 生成的矢量绘制代码。
- `Windows/main.c`：Windows 原生 Win32 / GDI+ 界面、图片裁剪和导出。
- `Windows/layout.h`：Windows 裁剪布局算法。
- `Assets/PlaneHub-logo.svg`：透明背景 Logo，字形已转路径。
- `Assets/generate-logo.py`：将 Logo SVG 转为 Swift 和 C 的原生矢量绘制代码。
- `Assets/AppIcon.png`、`Assets/AppIcon.icns`、`Windows/AppIcon.ico`：当前飞机图标。
- `releases/v1.3.0/`：当前最终安装文件及 SHA-256 校验值。

Logo 来源：[PlaneHub Figma 节点](https://www.figma.com/design/C7uFxoAeVmp7rQGN0HO6f0/EggPlane?node-id=219-19346)。图标由内置 imagegen 生成，提示词保存在 `Assets/icon-prompt.txt`。

## 构建 macOS

在 macOS 安装 Xcode Command Line Tools，然后在仓库根目录运行：

```sh
zsh build.sh
```

仅生成 App：`zsh build.sh --app-only`。
输出在 `dist/`。修改 AppIcon.png 后可先执行 `zsh build-icon.sh`，重新生成多尺寸 ICNS。

## 构建 Windows

使用官方 Zig 0.14.1，通过 macOS 的 zsh 交叉编译 Windows x64：

```sh
ZIG=/absolute/path/to/zig zsh Windows/build.sh
python3 Windows/verify_exe.py
```

也可将 `zig` 加入 PATH 后直接执行构建脚本。EXE 内嵌图标和清单，仅依赖 Windows 10/11 系统组件，无需另装运行库。

## 验证

```sh
# Mac：裁剪布局与 PNG 导出读取
"dist/staging/3比4图片快切.app/Contents/MacOS/RedNoteCrop" --self-test

# Windows 算法：可在 Mac 使用 clang 运行
clang Windows/test_layout.c -o /tmp/imagecutter-layout-test
/tmp/imagecutter-layout-test

# Windows 文件结构：x64 GUI、图标、版本、清单及依赖
python3 Windows/verify_exe.py
```

视觉检查覆盖 Mac 的完整 Logo、分割线和拆分三阶段，以及标题、Logo 和按钮对齐。Windows 仍需在实机上检查拖入、照片方向、两张/三张导出、文件夹权限、键盘导航与动画。
