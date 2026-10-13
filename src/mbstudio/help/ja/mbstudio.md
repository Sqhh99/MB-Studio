# mbstudio — MB-Studio デスクトップアプリとコマンドライン

`mbstudio` は MB-Studio デスクトップアプリを起動します。スワスソナーファイルの確認、GMT グリッドの陰影図表示、すべての MB-System プログラムの実行（出力をリアルタイム表示）、およびこのヘルプの閲覧を、英語・日本語・簡体字中国語で行えます。同じ実行ファイルはコマンドラインからも使用でき、プラグインの一覧表示、ヘルプページの表示、JSON を返す MB-Studio コマンドの実行ができます。

## 書式

    mbstudio [file ...]
    mbstudio --help-program NAME [--lang CODE]
    mbstudio --command ID [key=value ...]
    mbstudio --list-plugins | --list-commands | --list-services
    mbstudio --headless | --version | --help
    mbstudio --disable PLUGIN | --plugin-dir DIR

## 説明

オプションを付けずに実行するとメインウィンドウが開きます。コマンドラインで指定した、またはウィンドウにドロップしたスワスデータファイル、データリスト、GMT グリッドは「検査」または「グリッド」ページで開かれます。「プログラム」ページには `bin` フォルダーにインストールされた MB-System プログラムが一覧表示されます。プログラムを選ぶとヘルプページが表示され、引数と作業フォルダーを入力して実行できます。出力は「出力」タブに表示され、終了時に終了コードが示されます。

MB-Studio は小さなカーネルとプラグインで構成されています。`mbsystem` プラグインが MB-System のサービス（フォーマット、スワスファイル、グリッド、プログラム、ヘルプページ）を提供し、`qmlui` プラグインがユーザーインターフェースを提供します。サービスの機能はすべてコマンドとしても利用できるため、ユーザーインターフェースなしで `--command` を使ってスクリプトから呼び出せます。

ユーザーインターフェースの言語は Windows の表示言語に従います。サイドバーの言語ボタンで別の言語（English、日本語、简体中文）を選択でき、選択は保存されます。プログラムのヘルプページも同じ言語で表示されます。

## 主なオプション

- `--help-program NAME` — MB-System プログラム NAME（例: `mbgrid`）のヘルプページをテキストで表示して終了します。
- `--lang CODE` — ヘルプページの言語: `en`、`ja`、`zh_CN`。既定値は環境変数 `MBSTUDIO_LANG`、未設定の場合は Windows の表示言語です。
- `--command ID [key=value ...]` — MB-Studio コマンドを実行し、結果を JSON で出力します（例: `swath.inspect`、`grid.info`、`formats.list`、`tools.list`、`help.show`）。コマンドが成功すると終了ステータスは 0 です。
- `--list-plugins` — プラグインとその状態、バージョンを一覧表示します。
- `--list-commands` — `--command` で使用できるコマンドを一覧表示します。
- `--list-services` — プラグインが提供するサービスを一覧表示します。
- `--headless` — ユーザーインターフェースなしでカーネルとプラグインを起動します（動作確認用）。
- `--disable PLUGIN` — プラグイン PLUGIN を起動しません（複数指定可）。
- `--plugin-dir DIR` — フォルダー DIR からもプラグインを読み込みます（複数指定可）。
- `--version` — バージョンを表示して終了します。
- `--help` — 簡単な使い方を表示して終了します。

## 使用例

スワスファイルをデスクトップアプリで開きます。

    mbstudio 0001_20250101_000000.mb58

mbgrid の日本語ヘルプページをコンソールで読みます。

    mbstudio --help-program mbgrid --lang ja

スクリプトで使うために、スワスファイルの統計を JSON で取得します。

    mbstudio --command swath.inspect path=line1.mb88

両方のプラグインが読み込まれていることを確認します。

    mbstudio --list-plugins

## 注意

- 設定（言語、テーマ、最近使ったファイル、「プログラム」ページの作業フォルダー）は `%APPDATA%\MB-Studio\mbstudio.ini` に保存されます。
- `MBSTUDIO_LANG`（`en`、`ja`、`zh_CN`）を設定すると、ユーザーインターフェースと `--help-program` の両方の言語を固定できます。
- スタートメニューの「MB-System コマンドプロンプト」を開くと、`mbstudio`、MB-System プログラム、`gmt` を直接実行できるコンソールが起動します。

## 関連項目

`mbinfo`、`mbgrid`、`mbprocess`、`gmt`。すべてのプログラムのヘルプページは HTML 形式でもインストールされています（スタートメニュー: MB-System プログラム ヘルプ）。
