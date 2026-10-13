# mbsvpselect — スワスファイルごとに最適な音速プロファイルを選択

`mbsvpselect` は、調査の各スワスファイルについて、利用可能な音速プロファイル（SVP）の中から最も適切なものを選び、そのプロファイルを用いたレイトレーシングによる水深再計算を有効にします。ユーザーはスワスファイルのデータリスト、SVP ファイルのリスト、および選択方法（位置が最も近い、時刻が最も近い、またはその組み合わせ）を与えます。多数のキャストを含む調査で、`mbset` を使って手作業でファイルごとにプロファイルを割り当てるのが煩雑な場合に使用します。

## 書式

    mbsvpselect
        --check-zero-position {-N}
        --help {-H}
        --input=datalist {-Idatalist}
        --method=mode[/period_or_range[/seasonal]] {-Pmode[/period_or_range[/seasonal]]}
        --svplist=svplist {-Ssvplist}
        --verbose {-V}

## 説明

各スワスファイルの位置と時刻は `.inf` ファイルから読み込まれるため、補助ファイル `.inf`、`.fbt`、`.fnv` を事前に作成しておく必要があります（たとえば `mbdatalist -N`）。距離計算（WGS84 楕円体上の測地線）には各ファイルの中間点の位置が、時刻の比較には開始時刻が使われます。

SVP ファイルはデータリストと同様の svplist に列挙します。1 行に 1 つの SVP ファイルを書き、svplist から別の svplist を参照することもできます。各 SVP ファイルは `深度 音速`（メートル、メートル/秒）の組を並べたテキストファイルで、キャストの時刻と位置を示すヘッダ行を含みます。対応するヘッダは、`mbsvplist` が出力するもの（`## MB-SVP yyyy/mm/dd hh:mm:ss longitude latitude`）と CARIS のヘッダ（`Section yyyy-ddd hh:mm:ss latitude longitude`、位置は 度:分:秒）です。

`-P` で次の 5 つの選択方法を指定できます：
- `-P0` — 位置が最も近いもの（既定）。
- `-P1` — 時刻が最も近いもの。
- `-P2` または `-P2/period` — ファイルから指定時間（時間単位、既定 10）以内の SVP のうち位置が最も近いもの。期間内に SVP がない場合は位置が最も近いものを使用します。
- `-P3`、`-P3/range` または `-P3/range/0` — ファイルから指定距離（メートル、既定 10000）以内の SVP のうち時刻が最も近いもの。
- `-P3/range/1` — 指定距離以内の SVP のうち月が最も近いもの（年を問わない季節的な選択）。

各スワスファイルについて `mbset -I file -PSVPFILE:svpfile` を実行し、ファイルのパラメータ（`.par`）ファイルに SVP ファイルを設定してレイトレーシングを有効にします。スワスファイルと選択された SVP の組はカレントディレクトリの `result.txt` にも書き出されます。水深が実際に再計算されるのは、その後に `mbprocess` を実行したときです。

## 主なオプション

- `-Idatalist` — 処理するスワスファイルのデータリスト（ロング形式 `--input=datalist`）。既定値：`datalist.mb-1`。
- `-Ssvplist` — SVP ファイルのリスト（ロング形式 `--svplist=svplist`）。既定値：`svplist.mb-1`。
- `-Pmode[/period_or_range[/seasonal]]` — 上記の SVP 選択方法（ロング形式 `--method=...`）。時間は時間単位、距離はメートル単位です。既定値：`-P0`。
- `-N` — `.inf` の位置が誤って経度 0・緯度 0 になっているスワスファイルを検出し、その位置を距離計算に使わないようにします（ロング形式 `--check-zero-position`）。SVP の位置には影響しません。
- `-V` — 詳細モード（ロング形式 `--verbose`）。ファイルと SVP の情報、計算した距離や時間差を表示します。
- `-H` — 簡単な説明と使い方を表示して終了します（ロング形式 `--help`）。

## 使用例

データリスト `datalist.mb-1` にスワスファイルを、`svplist.mb-1` に SVP ファイルを 1 行に 1 つずつ列挙しておきます：

    svp_001.svp
    svp_002.svp
    svp_003.svp

まず補助ファイルを作成し、次に経度 0・緯度 0 の位置を検査しながら、各スワスファイルに 50 時間以内に取得された最も近い SVP を割り当てます：

    mbdatalist -I datalist.mb-1 -N
    mbsvpselect -N -V -I datalist.mb-1 -S svplist.mb-1 -P2/50

各ファイルから 5 km 以内の SVP のうち時刻が最も近いものを割り当て、再処理します：

    mbsvpselect -I datalist.mb-1 -S svplist.mb-1 -P3/5000
    mbprocess -I datalist.mb-1

## 注意

- 処理が終わると「End the program press ENTER」と表示して入力待ちになります。Enter キーを押すとプロンプトに戻ります。
- データリストとスワスファイルがあるディレクトリで実行してください。`result.txt` もそこに書き出されます。SVP ファイル名は svplist に書かれたとおりに `mbset` に渡されるため、相対名はカレントディレクトリから解決されます。
- `mbset` のコマンドラインは引用符なしで組み立てられるため、スワスファイルや SVP ファイルの名前・パスにスペースを含めないでください。
- `.inf` ファイルは最新である必要があります。`.inf` ファイルのないファイルは見つかりません。

## 関連項目

`mbset`、`mbprocess`、`mbsvplist`、`mbdatalist`、`mblevitus`。完全なリファレンスは MB-System のマニュアルページ mbsvpselect(1)（英語）です。
