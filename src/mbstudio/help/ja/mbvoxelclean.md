# mbvoxelclean — 3D ボクセル密度フィルタによるビームフラグ設定

`mbvoxelclean` は 3 次元の密度フィルタでスワス水深データをクリーニングします。測深点がまばらなボクセルに含まれる測深はノイズとみなしてフラグを立て、測深点が密集したボクセルは海底などの実在するターゲットとみなします。主に水中ライダーのような非常に高密度な点群を対象としたプログラムで、`mbprocess` の前の編集段階で使用します。

## 書式

    mbvoxelclean [--verbose --help --input=datalist --format=value
        --voxel-size=xysize[/zsize] --occupy-threshold=value
        --count-flagged --flag-empty --ignore-empty
        --unflag-occupied --ignore-occupied --neighborhood=value
        --range-minimum=value --range-maximum=value
        --acrosstrack-minimum=value --acrosstrack-maximum=value
        --amplitude-minimum=value --amplitude-maximum=value]

## 説明

入力は 1 つのスワスファイル、または複数のスワスファイルを参照するデータリストです。各ファイルは個別に読み込まれ、フィルタ処理されます。1 ファイル分のすべての測深点をメモリに読み込み、指定したサイズの立方体（または直方体）のボクセルに割り当てます。しきい値以上の測深点を含むボクセルは「占有」（有効なターゲット）、それ未満のボクセルは「空」と判定されます。

その後、次の 2 つの処理を適用できます。空のボクセル内でフラグが立っていない測深にフラグを立てる処理（既定、`--flag-empty`）と、占有されたボクセル内でフラグが立っている測深のフラグを解除する処理（`--unflag-occupied`、既定ではオフ）です。`--neighborhood` オプションを使うと占有領域を指定ボクセル数だけ拡張でき、密集領域の近くではフラグ設定が緩和されます。

データの読み込み時には、センサーからのレンジ、アクロストラック距離、ビーム振幅による単純なフィルタも任意で適用できます。これらのフィルタは密度フィルタより先に適用されるため、`--unflag-occupied` を指定すると、これらのフィルタで立てたフラグが解除されることがあります。

スワスファイル自体は変更されません。ビームフラグの変更はすべてそのスワスファイルの編集保存ファイル（`<file>.esf`）に書き込まれ（新規作成または追記）、`mbprocess` のパラメータファイル（`<file>.par`）も更新されるため、`mbprocess` 実行時に編集が適用されます。これは `mbclean` や対話型エディタが使用するものと同じ編集保存ファイルです。`.fbt`（高速水深）ファイルがある場合は、元のファイルの代わりにそちらを読み込みます。ロックファイルを使用している場合、他のプログラムがロックしているファイルはスキップされます。

## 主なオプション

- `--input=datalist` — 処理するスワスファイルまたはデータリスト。既定値: `datalist.mb-1`。
- `--format=value` — 入力の MBIO フォーマット ID。負の値の場合、入力はデータリストとみなされます。通常はファイル名から自動判別されます。
- `--voxel-size=xysize[/zsize]` — ボクセルの水平サイズと、任意で鉛直サイズ（メートル）。`zsize` を省略すると `xysize` と同じ値になります。既定値: 0.05 m。
- `--occupy-threshold=value` — ボクセルを占有と判定する最小測深点数。既定値: 5。
- `--count-flagged` — ボクセルの占有判定に、既にフラグが立っている測深も数えます（通常はフラグのない測深のみを数えます）。
- `--flag-empty` — 空のボクセル内のフラグのない測深にフラグを立てます（既定）。
- `--ignore-empty` — 空のボクセル内の測深を変更しません。
- `--unflag-occupied` — 占有ボクセル内のフラグ付き測深のフラグを解除します。
- `--ignore-occupied` — 占有ボクセル内のフラグ付き測深はフラグを付けたままにします（既定）。
- `--neighborhood=value` — 占有ボクセルから `value` ボクセル以内（x、y、z 方向）のボクセルも占有として扱います。既定値: 0。
- `--range-minimum=value`, `--range-maximum=value` — センサーからこのレンジ（m）より近い、または遠いフラグのない測深にフラグを立てます。
- `--acrosstrack-minimum=value`, `--acrosstrack-maximum=value` — アクロストラック距離（m）が最小値より左舷側、または最大値より右舷側にあるフラグのない測深にフラグを立てます。
- `--amplitude-minimum=value`, `--amplitude-maximum=value` — ビーム振幅が最小値未満、または最大値を超えるフラグのない測深にフラグを立てます。
- `--verbose` — ファイルごとおよび合計の、フラグ設定・解除されたビーム数を表示します。
- `--help` — 簡単なヘルプを表示して終了します。

## 使用例

データリストのすべてのファイルについて、4 cm のボクセルで測深点が 5 未満のボクセル内の測深にフラグを立て、進行状況を表示します。

    mbvoxelclean --input=datalist.mb-1 --voxel-size=0.04/0.04 --occupy-threshold=5 --flag-empty --verbose

10 cm のボクセルを使い、密集領域にある既存のフラグ付き測深を復元し、さらにセンサーから 30 m より遠い測深をすべてフラグ設定します。

    mbvoxelclean --input=datalist.mb-1 --voxel-size=0.1 --occupy-threshold=8 --unflag-occupied --range-maximum=30

その後、編集をデータに適用します。

    mbprocess -Idatalist.mb-1

## 注意

- 指定できるのはロングオプション（`--name=value`）のみで、1 文字のオプションはありません。
- 1 ファイル分のすべての測深点をメモリに保持するため、大きなファイルを非常に小さなボクセルで処理すると多くのメモリが必要です。
- 編集保存ファイルはカレントディレクトリではなく、スワスファイルと同じフォルダに書き込まれます。データリスト内のファイル名にはスペースを含めないでください。
- ビームフラグに対応していないデータフォーマットでは、`mbprocess` は編集された測深にフラグを立てる代わりにヌル（ゼロ）にします。その場合、プログラムは警告を表示します。

## 関連項目

`mbclean`, `mbareaclean`, `mbprocess`, `mbset`, `mbinfo`, `mbdatalist`。完全なリファレンスは MB-System のマニュアルページ mbvoxelclean(1)（英語）です。
