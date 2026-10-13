# mbdumpesf — 編集保存ファイル（.esf）のビーム編集を出力・抽出

`mbdumpesf` は、`mbclean`、`mbareaclean` や対話型編集で作成されたビームフラグを保持する MB-System の編集保存ファイル（`.esf`）を読み込み、すべての編集イベントをテキストで一覧表示します。選択した種類の編集を除外して、新しいバイナリの `.esf` ファイルにコピーすることもできます。ファイル内で何がフラグ付けされているかを確認したり、`mbprocess` の実行前に特定の種類の編集を取り除いたりするときに使います。

## 書式

    mbdumpesf --input=esffile
        [--output=esffile --ignore-unflag --ignore-flag
        --ignore-filter --ignore-zero
        --verbose --help]

## 説明

MB-System では、ビームフラグは生のスワスファイルには書き込まれません。スワスファイル名に `.esf` を付けた名前のバイナリの「編集保存ファイル」（例: `0001_20250101_000000.all.esf`）に編集イベントとして保存され、後で `mbprocess` によって適用されます。各イベントはピングの時刻、ビーム番号、アクションを持ちます。

    action=1: flag manually
    action=2: unflag
    action=3: zero - make beam null
    action=4: flag by autofilter

`--output` を指定しない場合、各イベントは標準出力に 1 行ずつ書き出されます。例: `EDITS READ: i:0 time: 947562291.799000 2008/04/22 22:26:36.799000 beam:21 action:1`。時刻はエポック秒（1970-01-01 からの秒数）と日付・時刻の両方で表示されます。

`--output=esffile` を指定すると、イベントはテキストの代わりに新しい編集保存ファイルにバイナリ形式で書き出されます。`--ignore-...` オプションを指定すると、対応する種類のイベントが出力から除外されます。テキストモードでは除外されたイベントも `** EDITS READ BUT IGNORED **` という印を付けて表示されます。`--verbose` を指定すると、最後に編集の種類ごとの合計が表示されます。

## 主なオプション

- `--input=esffile`（または `-Iesffile`）— 読み込む編集保存ファイル。必須です。
- `--output=esffile` — （残った）編集をテキスト表示する代わりに、このバイナリ編集保存ファイルに書き出します。
- `--ignore-flag` — 手動フラグのイベント（action 1）を除外します。
- `--ignore-unflag` — フラグ解除のイベント（action 2）を除外します。
- `--ignore-zero` — ゼロ化（ビーム無効化）のイベント（action 3）を除外します。
- `--ignore-filter` — 自動フィルタによるフラグのイベント（action 4）を除外します。
- `--verbose`（または `-V`）— 読み込んだ編集と除外した編集の合計を表示します。
- `--help`（または `-H`）— ヘルプを表示して終了します。

## 使用例

1 つのファイルのすべての編集イベントを画面に表示します。

    mbdumpesf -I survey\0001_20250101_000000.all.esf

一覧をテキストファイルに保存します。

    mbdumpesf -I survey\0001_20250101_000000.all.esf > edits.txt

自動フィルタのフラグを除いた編集のコピーを作成し、合計を表示します。

    mbdumpesf --input=0001_20250101_000000.all.esf --output=manual_only.esf --ignore-filter --verbose

## 注意

- 短い形式があるのは `-I`、`-V`、`-H` だけです。出力と除外のオプションは `--` で始まる長い形式で指定してください。
- 抽出したコピーを使うには、元のファイルのバックアップを取ったうえで、スワスファイル名に `.esf` を付けた名前に変更し、`mbprocess` を再実行します。
- 空白を含むファイル名は引用符で囲んでください。

## 関連項目

`mbclean`、`mbareaclean`、`mbprocess`、`mbgetesf`、`mbvoxelclean`、`mbset`。完全なリファレンスは MB-System のマニュアルページ mbdumpesf(1)（英語）です。
