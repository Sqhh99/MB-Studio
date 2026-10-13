# mbstudio — MB-Studio 桌面应用程序与命令行前端

`mbstudio` 用于启动 MB-Studio 桌面应用程序：查看条带声呐文件、以晕渲地形图显示 GMT 网格、运行所有 MB-System 程序并实时显示输出，以及阅读本帮助文档，支持英文、日文和简体中文。同一个可执行文件也可在命令行中使用：列出插件、显示帮助页面，以及运行返回 JSON 结果的 MB-Studio 命令。

## 语法

    mbstudio [file ...]
    mbstudio --help-program NAME [--lang CODE]
    mbstudio --command ID [key=value ...]
    mbstudio --list-plugins | --list-commands | --list-services
    mbstudio --headless | --version | --help
    mbstudio --disable PLUGIN | --plugin-dir DIR

## 说明

不带选项运行时，`mbstudio` 打开主窗口。在命令行中指定或拖放到窗口上的条带数据文件、数据列表和 GMT 网格，会在“检查”或“网格”页面中打开。“程序”页面列出安装在 `bin` 文件夹中的 MB-System 程序；选择一个程序即可阅读其帮助页面，填写参数和工作目录后运行。输出显示在“输出”选项卡中，程序结束时显示退出代码。

MB-Studio 由一个小型内核和若干插件组成：`mbsystem` 插件提供 MB-System 服务（数据格式、条带文件、网格、程序和帮助页面），`qmlui` 插件提供用户界面。所有服务功能也都以命令的形式提供，因此无需用户界面即可通过 `--command` 在脚本中调用。

用户界面的语言跟随 Windows 的显示语言；可通过侧边栏中的语言按钮选择其他语言（English、日本語、简体中文），所选语言会被保存。程序的帮助页面也以相同语言显示。

## 常用选项

- `--help-program NAME` — 以文本形式显示 MB-System 程序 NAME（例如 `mbgrid`）的帮助页面，然后退出。
- `--lang CODE` — 帮助页面的语言：`en`、`ja` 或 `zh_CN`。默认值为环境变量 `MBSTUDIO_LANG`，未设置时使用 Windows 的显示语言。
- `--command ID [key=value ...]` — 运行 MB-Studio 命令并以 JSON 输出结果，例如 `swath.inspect`、`grid.info`、`formats.list`、`tools.list`、`help.show`。命令成功时退出状态为 0。
- `--list-plugins` — 列出插件及其状态和版本。
- `--list-commands` — 列出可用于 `--command` 的命令。
- `--list-services` — 列出插件提供的服务。
- `--headless` — 不启动用户界面，仅启动内核和插件（用于检查）。
- `--disable PLUGIN` — 不启动插件 PLUGIN（可重复指定）。
- `--plugin-dir DIR` — 同时从文件夹 DIR 加载插件（可重复指定）。
- `--version` — 显示版本号后退出。
- `--help` — 显示简短的用法说明后退出。

## 示例

在桌面应用程序中打开一个条带文件：

    mbstudio 0001_20250101_000000.mb58

在控制台中阅读 mbgrid 的日文帮助页面：

    mbstudio --help-program mbgrid --lang ja

以 JSON 格式获取条带文件的统计信息，供脚本使用：

    mbstudio --command swath.inspect path=line1.mb88

检查两个插件是否均已加载：

    mbstudio --list-plugins

## 注意

- 设置（语言、主题、最近使用的文件、“程序”页面的工作目录）保存在 `%APPDATA%\MB-Studio\mbstudio.ini` 中。
- 设置 `MBSTUDIO_LANG`（`en`、`ja` 或 `zh_CN`）可同时固定用户界面和 `--help-program` 的语言。
- 开始菜单中的“MB-System 命令提示符”会打开一个控制台，可在其中直接运行 `mbstudio`、MB-System 程序和 `gmt`。

## 另请参阅

`mbinfo`、`mbgrid`、`mbprocess`、`gmt`。所有程序的帮助页面也以 HTML 格式安装（开始菜单：MB-System 程序帮助）。
