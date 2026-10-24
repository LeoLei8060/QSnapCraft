# QSnapCraft
基于Qt的快速、高效的截图工具。
![example](./ReadMe_res/example.gif)

## 功能开发状态

| 功能模块 | 子功能 | 状态 | 说明 |
|---------|--------|------|------|
| 截屏功能 | 手动框选截屏 | ✅ | 支持多显示器 |
|         | 智能捕获组件 | ✅ | 自动识别窗口和控件 |
| 画板编辑 | 矩形标注 | ✅ | |
|         | 画笔颜色设置 | ✅ | |
|         | 圆形标注 | ✅ | |
|         | 折线标注 | ✅ | |
|         | 箭头标注 | ✅ | |
|         | 画笔 | ✅ | |
|         | 记号笔 | ✅ | |
|         | 文字标注 | ✅ | |
|         | 马赛克 | ✅ | |
|         | 橡皮擦 | ✅ | |
|         | 撤销/重做 | ✅ | |
| Pin截图 | 置顶显示 | ✅ | |
|         | 拖动位置 | ✅ | |
|         | 调整大小 | ✅ | |
|         | 双击关闭 | ✅ | |
| 扩展功能 | 截图文本识别（OCR） | ✅ | 使用 Windows OCR，可检测已安装语言包 |
|         | 滚动截图 | ✅ | 自动识别纵向和横向滚动 |
|         | 屏幕标尺 | ✅ | 横竖标尺、刻度测距、矩形与点距测量 |
| 输出 | 手动保存、快捷保存、自动保存 | ✅ | 默认保存到系统下载目录，可分别配置 |

## 构建要求

- Qt 6.7.2 或更高版本
- MSVC 2019 或更高版本
- CMake 3.24 或更高版本
- Windows 10/11 操作系统

## 发布构建

项目提供可重复的 Windows x64 便携版打包脚本：

```powershell
.\scripts\package-release.ps1
```

脚本会执行 Release 构建、全部 CTest、CMake 安装、Qt 运行库部署和 SHA-256 校验，
最终在 `dist` 目录生成 `QSnapCraft-<version>-win64.zip`。发布包采用便携模式，
配置文件和贴图数据写入程序所在工作目录；请将程序解压到当前用户可写的目录。

便携包的长稳冒烟测试可执行：

```powershell
.\scripts\run-release-soak.ps1
```

运行日志位于 `%LOCALAPPDATA%\QSnapCraft\QSnapCraft\logs`。日志不会主动记录截图、
剪贴板内容或完整用户目录。

详细功能边界、系统要求和已知限制见 [RELEASE_NOTES.md](RELEASE_NOTES.md)。

## 许可证

项目源码使用 MIT License。发布包同时包含 Qt 和 Microsoft Visual C++ 运行库的
第三方许可说明，详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

## 项目结构

```
QSnapCraft/
├── src/                    # 源代码目录
│   ├── app/               # 应用启动与窗口协调
│   ├── capture/           # 截图请求、选区与会话
│   ├── config/            # 配置模型与持久化
│   ├── output/            # 复制、保存与贴图输出
│   ├── paint/             # 标注场景与图元
│   ├── pinboard/          # 贴图窗口
│   ├── platform/windows/  # Windows 平台能力
│   └── ui/                # 工具条、覆盖层、设置和托盘
├── resources/             # 资源文件
└── CMakeLists.txt        # CMake构建配置
```
