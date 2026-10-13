# mbsegygrid — SEGY トレースを地震探査断面にグリッド化

`mbsegygrid` は SEGY ファイルのトレースを、地震探査またはサブボトムプロファイラの
断面を表す GMT netCDF グリッドに変換します。x 軸はトレース番号（または距離）、
y 軸は往復走時（または深度）です。`mbextractsegy` の後に、地層探査測線の断面
画像を作成し `gmt` でプロットするために使用します。

## 書式

    mbsegygrid -Ifile -Oroot
        --agc=agcmaxvalue/agcwindow {-Bagcmaxvalue/agcwindow}
        --decimation=decimatex/decimatey {-Ddecimatex/decimatey}
        --distance-plot=distancebin/startlon/endlon/startlat/endlat {-Rdistancebin/startlon/endlon/startlat/endlat}
        --filter=filtermode/filterwindow {-Ffiltermode/filterwindow}
        --gain=gainmode/gain[/gainwindow[/gaindelay]] {-Ggainmode/gain[/gainwindow[/gaindelay]]}
        --geometry=geometrymode {-Cgeometrymode}
        --help {-H}
        --input=file {-Ifile}
        --output=fileroot {-Ofileroot}
        --scale-to-distance=shotscale/timescale {-Ashotscale/timescale}
        --time-sweep=timesweep[/timedelay] {-Ttimesweep[/timedelay]}
        --trace-mode=tracemode[/tracestart/traceend[/chanstart/chanend]] {-Stracemode[/tracestart/traceend[/chanstart/chanend]]}
        --verbose {-V}
        --window=windowmode/windowstart/windowend {-Wwindowmode/windowstart/windowend}

## 説明

出力グリッドは `<root>.grd` に書き出されます。グリッドの各列が 1 本のトレース
（`-D` による間引き後）、各行が 1 つの時間サンプルに対応します。走時は負の値で
格納されるため、遅い到達ほど断面の下側に表示されます。データのないセルは NaN に
なります。

トレース範囲（`-S`）や時間スイープ（`-T`）を指定しない場合は、SEGY ファイルの
`.sinf` ファイルから値を取得し、すべてのトレースとサンプルをグリッド化します。
`.sinf` ファイルがない場合や古い場合は、`mbsegyinfo -O` を実行して自動的に
作成します。

グリッド化の前に、トレースに時間可変ゲイン（`-G`）、コサインローパスフィルタ
（`-F`）、自動ゲイン制御（`-B`）、必要なサンプルだけを残す時間ウィンドウ
（`-W`）を適用できます。`-R` を指定すると、トレースは 2 点間の直線上に投影され、
トレース番号ではなく直線に沿った距離でビン分けされます。`-A` を指定すると、
軸をメートル単位の距離と深度に換算します。

## 主なオプション

- `-I<file>` — 入力 SEGY ファイル。必須。
- `-O<root>` — 出力名のルート。グリッドは `<root>.grd` に書き出されます。必須。
- `-S<mode>[/<start>/<end>[/<schan>/<echan>]]` — グリッド化するトレース。
  モード `0`：`start` と `end` はショット番号、モード `1`：CMP（RP、CDP）番号、
  モード `2`：全チャンネルをまとめたショット番号。`schan`/`echan` は
  マルチチャンネルデータのチャンネルを選択します。既定：全トレース。
- `-T<sweep>[/<delay>]` — グリッド化する時間範囲の長さ（秒）とその開始時刻。
  既定：トレース全長。
- `-D<decimatex>/<decimatey>` — 各グリッドセル内で平均化してトレース数と
  サンプル数を減らします。既定：`1/1`。
- `-G<mode>/<gain>[/<window>[/<delay>]]` — ゲイン：モード `0` なし、`1` トレース
  開始から、`2` 海底到達から。各サンプルに (1 + gain * t) を掛けます。`window` は
  ゲインをかける区間を制限し（それ以降のサンプルは 0 になります）、`delay` は
  ゲイン開始をずらします（秒）。モード `3` は、海底の前後 `window` 内の最大値が
  `gain` になるように各トレースをスケーリングします。
- `-B<maxvalue>[/<window>]` — 自動ゲイン制御：`window` 秒の移動窓内の最大値が
  `maxvalue` になるように各サンプルをスケーリングします。`window` を省略すると
  トレース全体を正規化します。
- `-F<mode>/<window>` — ローパスフィルタ。モード `1` は `window` 秒のコサイン
  テーパーフィルタです。
- `-W<mode>/<start>/<end>` — 各トレースの一部だけをグリッド化します。モード `1`
  絶対時刻、`2` 海底到達時刻からの相対時刻（start は負の値が多い）、`3` ソナー
  深度に対応する時刻からの相対時刻。
- `-R<distancebin>/<startlon>/<endlon>/<startlat>/<endlat>` — 2 点間の直線に
  沿った距離で、`distancebin` メートルのビンごとにグリッド化します。
- `-A<shotscale>/<timescale>` — x をショットからメートル（ショット間隔 m）に、
  y を秒からメートル（水中では通常 750 m/s）に換算します。
- `-C<mode>` — ジオメトリ：`0` 鉛直トレース（既定）、`1` 記録されたピッチで
  サンプル位置を補正。
- `-V` — 詳細表示。進捗メッセージを多く表示します。

## 使用例

SEGY ファイルの全トレースと全サンプルを `sbp_0001.grd` にグリッド化します。

    mbsegygrid -I sbp_0001_env.segy -O sbp_0001

ショット 1500～2000 を、ショット後 0.25 秒から 0.1 秒間グリッド化します。

    mbsegygrid -I sbp_0001_env.segy -O sbp_0001_part -S0/1500/2000 -T0.1/0.25

海底から始まるゲインをかけ、海底の 0.02 秒上から 0.15 秒下までだけを残します。

    mbsegygrid -I sbp_0001_env.segy -O sbp_0001_gain -G2/5.0 -W2/-0.02/0.15

ショット間隔 0.5 m、750 m/s として断面をメートル単位で表示します。

    mbsegygrid -I sbp_0001_env.segy -O sbp_0001_m -A0.5/750

## 注意

- プログラムは最後にプロットマクロ `mbm_grdplot` を呼び出しますが、これは
  Windows では利用できません。この処理はメッセージを表示してスキップされ、
  グリッドは通常どおり書き出されます。グリッドは `gmt grdimage` でプロット
  してください。
- `-O` にパスを含めない限り、出力ファイルはカレントディレクトリに書き出され
  ます。空白を含むパスは引用符で囲んでください。

## 関連項目

`mbextractsegy`、`mbsegyinfo`、`mbsegylist`、`mbsegypsd`、`gmt`。完全な
リファレンスは MB-System のマニュアルページ mbsegygrid(1)（英語）です。
