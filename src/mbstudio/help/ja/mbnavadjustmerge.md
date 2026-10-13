# mbnavadjustmerge — MBnavadjust 航法調整プロジェクトの作成・編集・統合

`mbnavadjustmerge` は MBnavadjust プロジェクトを操作するコマンドラインツールです。MBnavadjust は、測線が重なる・交差する箇所の海底地形を一致させることでスワスデータの航法を調整します。本プログラムでは、プロジェクトの作成、スワスデータの取り込み、交差の検出、航法タイの自動ピック、補正航法の逆解析、スワスファイルへの航法の適用、プロジェクトの編集・統合ができます。航法調整の作業をスクリプト化でき、また対話型の `mbnavadjust` が利用できない Windows では MBnavadjust プロジェクトを扱う手段となります。

## 書式

    mbnavadjustmerge --input=project_path [--input=project_path] [--output=project_path]
    mbnavadjustmerge --create-project=project_path [settings] [commands]
    mbnavadjustmerge --input=project_path [settings] [commands] [--verbose] [--help]

    settings:  --section-length=km --section-soundings=count --contour-interval=meters
               --color-interval=meters --tick-interval=meters --label-interval=meters
               --decimation=value --smoothing=value --zoffsetwidth=meters
    commands:  --import=path[:format] --import-as-survey=path[:format] --find-crossings
               --autopick --autopick-horizontal
               --autopick-crossing-type=all|mediocre|good|better|true
               --autopick-scope=all|survey|withsurvey|block|file|withfile|withsection
               --autopick-survey=survey --autopick-survey2=survey --autopick-file=file
               --autopick-section=section --autopick-overlap-threshold=percent
               --invert-navigation --update-grids --apply-navigation
               --set-global-tie=file:section[:snav]/xoffset/yoffset/zoffset[/xsigma/ysigma/zsigma]
               --set-tie=file1:section1/file2:section2/xoffset/yoffset/zoffset[/xsigma/ysigma/zsigma]
               --set-ties-xyz-all --set-ties-xyonly-all --set-ties-zonly-all
               --unset-tie=file1:section1/file2:section2 --skip-unset-crossings
               --update-file=file --update-all-files
               --import-tie-list=file --export-tie-list=file
               --triangulate --triangulate-all ...

## 説明

プロジェクトは `.nvh` ファイルまたは `.dir` ディレクトリのパスで指定します。`--create-project` は新しい空のプロジェクトを作成し（`--input` とは併用できません）、`--input` は既存のプロジェクトを開きます。設定オプション（セクション長とセクション内の測深点数、等深線表示、スムージング、鉛直オフセット探索幅）はプロジェクト作成時にも後からでも指定でき、常に他のコマンドより先に適用されます。

`--import` はスワスデータを追加します。データリスト（フォーマット指定なし）または単一ファイルとそのフォーマット ID（`file:format`）を指定します。各ファイルは現在のセクション設定に従ってセクションに分割され、取り込み済みデータとの交差が自動的に検出されます。時間的に連続するファイルは同じサーベイになり、時間の空きがあると新しいサーベイになります（`--import-as-survey` を使う場合を除く）。

`--autopick` は未解析の各交差で 3 次元（東・北・鉛直）の水深ミスフィットグリッドを計算し、最小値が明確な場合にその位置にタイを設定します。`--autopick-horizontal` は水平オフセットだけを使います。オートピック用のフィルタで対象とする交差を絞り込めます（既定では重なりが 10% 以上のすべての交差）。`--invert-navigation` はタイとグローバルタイのネットワークを解いてプロジェクト内に補正航法モデルを求め、`--apply-navigation` はその解をスワスファイルの処理パラメータに書き込み、次の `mbprocess` 実行で適用されるようにします。`--update-grids` は現在の航法からプロジェクトの参照水深グリッドを作り直します。

`--input` を 2 つ指定すると、2 番目のプロジェクトが 1 番目に追加されます。`--output` を指定すると、両方が新しい統合プロジェクトにコピーされ、作成済みのタイも引き継がれます。複数のコマンドを指定した場合は、コマンドラインに現れた順に実行されます。ファイル、セクション、サーベイはプロジェクト内の番号で指定します（例: `3:0` はファイル 3 のセクション 0）。

## 主なオプション

- `--input=project_path` — 既存のプロジェクト（`.nvh` ファイルまたは `.dir` ディレクトリ）。2 回指定すると 2 つのプロジェクトを統合します。
- `--output=project_path` — 統合した入力プロジェクトを受け取る新しいプロジェクト。省略すると 2 番目の入力プロジェクトが 1 番目に追加されます。
- `--create-project=project_path` — 新しい空のプロジェクトを作成します。同じ行の他のコマンドはこのプロジェクトに適用されます。
- `--section-length=km` — 以後に取り込むファイルの最大セクション長（km）。
- `--section-soundings=count` — 以後に取り込むファイルの 1 セクションあたりの最大測深点数。
- `--smoothing=value` — 航法逆解析のスムージングの重み（既定値 3.0、有効範囲はおよそ 0.1～10）。
- `--zoffsetwidth=meters` — ミスフィットグリッドとオートピックで使う鉛直オフセット探索範囲の幅。
- `--import=path[:format]` — データリスト、またはフォーマット ID を付けた単一のスワスファイルを取り込みます。繰り返し指定できます。
- `--import-as-survey=path[:format]` — `--import` と同様ですが、時間の空きに関係なく取り込んだ全ファイルを 1 つの新しいサーベイにします。
- `--find-crossings` — 新しい交差を検索し、重なりを再計算します（プロジェクトやサーベイの統合後、不連続点の変更後に有用です）。
- `--autopick` — 未解析の交差でタイ（3 次元オフセット）を自動ピックします。`--autopick-horizontal` は水平オフセットだけをピックします。
- `--autopick-crossing-type=all|mediocre|good|better|true` — 重なりが 10%、25%、50% 以上の交差、または航跡が実際に交差する交差だけを対象にします。
- `--autopick-scope=all|survey|withsurvey|block|file|withfile|withsection` — オートピックの対象をサーベイ、2 つのサーベイの組（`block`）、ファイル、セクションに限定します。対象は `--autopick-survey`、`--autopick-survey2`、`--autopick-file`、`--autopick-section` で指定します。
- `--autopick-overlap-threshold=percent` — オートピックの対象とする交差の最小重なり率（既定値 10）。
- `--invert-navigation` — タイとグローバルタイを逆解析して補正航法モデルを求めます。
- `--apply-navigation` — 航法解をスワスファイルの処理パラメータに書き込みます（`--invert-navigation` の後に実行し、続いて `mbprocess` を実行します）。
- `--update-grids` — 現在の航法からプロジェクトの参照水深グリッドを作り直します。
- `--set-global-tie=file:section[:snav]/xoffset/yoffset/zoffset[/xsigma/ysigma/zsigma]` — 航法点を絶対座標系（GPS など）に結び付けます。オフセットの単位は m です。
- `--set-tie=file1:section1/file2:section2/xoffset/yoffset/zoffset[/xsigma/ysigma/zsigma]` — 2 つのセクション間のタイを追加または置換します（必要なら交差も作成されます）。
- `--set-ties-xyz-all`、`--set-ties-xyonly-all`、`--set-ties-zonly-all` — すべてのタイのモードを設定します（ファイル、サーベイ、ブロック単位の版もあります。例: `--set-ties-zonly-by-block=survey1/survey2`）。
- `--unset-tie=file1:section1/file2:section2` — タイを 1 つ削除します（`--unset-ties-with-file=file`、`--unset-ties-by-block=survey1/survey2` もあります）。
- `--skip-unset-crossings` — 未解析の交差をすべてスキップ扱いにします。
- `--update-file=file`、`--update-all-files` — 再編集後に、プロジェクトに保存されたビームフラグを現在の処理済みスワスファイルから更新します。
- `--export-tie-list=file`、`--import-tie-list=file` — タイをテキストファイルに書き出す、または別のプロジェクトから書き出したタイを取り込みます。
- `--triangulate` — 等深線描画に使うセクションの三角形メッシュを事前に作成します（`--triangulate-all` はすべて作り直します）。
- `--verbose` — より多くの情報を表示します。`--help` で全オプションの一覧を表示します。

## 使用例

プロジェクトの作成、セクション長 0.1 km の設定、サーベイのデータリストの取り込みを一度に行います：

    mbnavadjustmerge --create-project=Survey2025.nvh --section-length=0.1 --import=datalist.mb-1

重なり 25% 以上の交差でタイを自動ピックし、確認用にタイを書き出します：

    mbnavadjustmerge --input=Survey2025.nvh --autopick --autopick-crossing-type=good --export-tie-list=ties_autopick.txt

補正航法を逆解析してスワスファイルに適用し、再処理します：

    mbnavadjustmerge --input=Survey2025.nvh --invert-navigation --apply-navigation
    mbprocess -I datalist.mb-1

既存の 2 つのプロジェクトを新しいプロジェクトに統合します：

    mbnavadjustmerge --input=Navadjust20140515m1.nvh --input=Navadjust20140515m2.nvh --output=Navadjust20140515All.nvh

## 注意

- 監視なしのオートピックはしばしば誤ります。逆解析の結果を使う前に（例えば `--export-tie-list` で）確認し、誤ったタイを削除または修正してください。
- 単一のスワスファイルを `--import` で取り込む場合はフォーマット ID が必要です（例: `--import=0001_20250101_000000.all:58`）。ファイル名の拡張子からは推定されません。
- `--update-grids` とプロジェクトの統合は、補助コマンド（`mbdatalist`、`mbprocess`、`mbgrid` やファイル操作ユーティリティ）をシステムのシェル経由で実行します。Windows ではこれらの処理が完了しない場合があります。参照グリッドが更新されない場合は `mbgrid` で作成してください。
- このビルドでは `--unset-ties-all` とその別名 `--unset-all-ties` は値を必要とします。`--unset-ties-all=1` のように指定してください（値は無視されます）。
- スペースを含むプロジェクトやデータのパスは引用符で囲んでください。

## 関連項目

`mbprocess`, `mbset`, `mbdatalist`, `mbgrid`, `mbnavlist`, `mbnavadjust`（対話型、Windows では利用不可）。完全なリファレンスは MB-System のマニュアルページ mbnavadjustmerge(1)（英語）です。
