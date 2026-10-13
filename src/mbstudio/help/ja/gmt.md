# gmt — MB-System 描画モジュール付きの GMT

`gmt` は Generic Mapping Tools（GMT 6）のドライバーで、MB-System の GMT サプリメントとともに MB-Studio にインストールされます。標準の GMT モジュール（`grdimage`、`coast`、`basemap`、`grdcontour` など）に加えて、スワスデータやグリッドを直接描画する MB-System モジュール `mbswath`、`mbcontour`、`mbgrdtiff` を実行できます。`mbgrid` や `mbmosaic` で作成したグリッド、およびスワスデータそのものの地図を作成するときに使用します。

## 書式

    gmt MODULE [module options]
    gmt mbswath -I datalist -J... -R... [options]
    gmt mbcontour -I datalist -J... -R... [options]
    gmt mbgrdtiff -I grid -O geotiff [options]
    gmt --version | --help

## 説明

各 GMT プログラムは `gmt` ドライバーのモジュールです。最初の引数でモジュールを指定し、その後にモジュールのオプションを続けます。オプションなしで `gmt MODULE` を実行すると、そのモジュールの使い方が表示されます（例: `gmt mbswath`）。

GMT 6 には 2 つのモードがあります。モダンモードでは、図は 1 つのセッション（`gmt begin map png`、描画コマンド、`gmt end show`）として作成します。1 行形式では、1 つのコマンドに図の形式とファイル名（`-png map`）を付け加えます。クラシックモードでは、各モジュールが PostScript を標準出力に書き出し、それをファイルにリダイレクト（`> map.ps`）します。複数のレイヤーを重ねるには `-K` と `-O` を使います。どちらのモードも Windows のコマンドプロンプトで動作します。

MB-System モジュールはインストール先の `bin\gmt_plugins` フォルダーから読み込まれるため、設定は不要です。GMT は自身のデータ（カラーテーブル、フォント、インストールされている場合は海岸線）を `share\gmt` から読み込みます。

## 主なオプション

- `MODULE` — 実行する GMT または MB-System モジュールの名前（例: `grdimage`、`mbswath`）。
- `--version` — GMT のバージョンを表示して終了します。
- `--help` — GMT モジュールの一覧を表示して終了します。
- `--show-modules` — MB-System モジュールを含む、利用可能なモジュールを一覧表示します。
- `--show-plugindir` — サプリメントモジュールを検索するフォルダーを表示します。
- `--show-sharedir` — GMT データファイルのフォルダーを表示します。

## 使用例

データリスト内のスワス水深の陰影図を swath.png に出力します（モダンモードの 1 行形式）。

    gmt mbswath -I datalist.mb-1 -JM15c -R-122.1/-121.9/36.7/36.9 -Z2 -png swath

mbgrid で作成したグリッドのカラー画像を作成します。

    gmt grdimage bathymetry.grd -JM15c -Cgeo -B -png bathymetry

GMT が認識している MB-System モジュールを一覧表示します。

    gmt --show-modules

## 注意

- 空白を含むファイル名は引用符で囲んでください（例: `-I "D:\Survey 2025\datalist.mb-1"`）。
- クラシックモードの PostScript はファイルにリダイレクト（`> map.ps`）する必要があります。MB-Studio の「プログラム」ページではリダイレクトを使えないため、モダンモードの 1 行形式（`-png name`）を使用してください。
- GMT の詳しいドキュメントは docs.generic-mapping-tools.org にあります。

## 関連項目

`mbswath`、`mbcontour`、`mbgrdtiff`、`mbgrid`、`mbmosaic`。
