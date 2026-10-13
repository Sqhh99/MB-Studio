# mbsegypsd — SEGY トレースのパワースペクトル密度をグリッド化

`mbsegypsd` は SEGY ファイルの各トレースのパワースペクトル密度（PSD）を計算し、
x 軸をトレース番号、y 軸を周波数とする GMT netCDF グリッド（ソノグラム）として
書き出します。サブボトムプロファイラやハイドロフォンのデータの周波数成分を
調べ、雑音源の特定や送信波形の確認に使用します。

## 書式

    mbsegypsd
        --decimate=decimatex {-Ddecimatex}
        --fft-length=nfft {-Nnfft}
        --help {-H}
        --input=file {-Ifile}
        --log-scale {-L}
        --output=root {-Oroot}
        --shot-scale=shotscale[/frequencyscale] {-Ashotscale[/frequencyscale]}
        --time-sweep=sweep[/delay] {-Tsweep[/delay]}
        --trace-range=mode[/start/end[/schan/echan]] {-Smode[/start/end[/schan/echan]]}
        --verbose {-V}
        --window=mode/start/end {-Wmode/start/end}

## 説明

各トレースの PSD はアンサンブル平均で推定されます。トレースを `nfft` サンプル
（`-N`、既定 1024）の区間に分け、各区間にハニング（コサイン）テーパーを掛けて
高速フーリエ変換を計算し、全区間のスペクトルを平均します。推定値はテーパーに
よる信号の損失を補正して正規化されます。周波数軸は 0 Hz からナイキスト周波数
1/(2 * サンプル間隔) まで、`nfft`/2 + 1 段階です。

書き出されるファイルは次の 2 つです。

- `<root>.grd` — PSD グリッド。単位は Intensity/Hz、`-L` 指定時は dB/Hz
  （PSD の 20 * log10）。
- `<root>_psd.txt` — 全トレースで平均した PSD。周波数と PSD 値のテキスト行。

トレース範囲（`-S`）や時間スイープ（`-T`）を指定しない場合は、すべての
トレースとサンプルが使われます。範囲は SEGY ファイルの `.sinf` ファイルから
読み込まれ、`.sinf` ファイルがない場合や古い場合は `mbsegyinfo -O` で自動的に
作成されます。

## 主なオプション

- `-I<file>` — 入力 SEGY ファイル。必須。
- `-O<root>` — `.grd` と `_psd.txt` ファイルの出力名ルート。必須。
- `-N<nfft>` — FFT 長（サンプル数）。既定：1024。
- `-L` — グリッドを線形単位ではなく dB/Hz で書き出します。
- `-S<mode>[/<start>/<end>[/<schan>/<echan>]]` — 処理するトレース。モード `0`：
  `start` と `end` はショット番号、モード `1`：CMP（RP、CDP）番号。
  `schan`/`echan` はマルチチャンネルデータのチャンネルを選択します。
  既定：全トレース。
- `-T<sweep>[/<delay>]` — 使用する時間範囲の長さと開始時刻（秒）。
- `-W<mode>/<start>/<end>` — 各トレースの一部だけを使用します。モード `1`
  絶対時刻、`2` 海底到達時刻からの相対時刻、`3` ソナー深度に対応する時刻から
  の相対時刻。
- `-D<decimatex>` — 出力グリッドのトレース間引き。既定：1。
- `-A<shotscale>/<frequencyscale>` — ショット間隔（m）を使って x 軸を
  ショット番号から距離に換算します。両方の値を指定する必要があります。
- `-V` — 詳細表示。進捗メッセージを多く表示します。

## 使用例

1024 点 FFT で dB/Hz のソノグラムを計算します（`testpsd.grd` と
`testpsd_psd.txt` が作成されます）。

    mbsegypsd -I hydrophone_ch025.segy -N1024 -L -O testpsd -V

周波数分解能を上げるため FFT を長くし、ショット 100～500 だけを処理します。

    mbsegypsd -I sbp_0001_env.segy -N4096 -L -S0/100/500 -O sbp_0001_psd

## 注意

- プログラムは最後にプロットマクロ `mbm_grdplot` と `mbm_xyplot` を呼び出し
  ますが、これらは Windows では利用できません。この処理はメッセージを表示して
  スキップされ、出力ファイルは通常どおり書き出されます。グリッドは
  `gmt grdimage` で、平均スペクトルは `gmt plot` でプロットしてください。
- `-A` の `frequencyscale` は記録されますが、周波数軸は変わりません。

## 関連項目

`mbsegygrid`、`mbsegyinfo`、`mbsegylist`、`mbextractsegy`、`gmt`。完全な
リファレンスは MB-System のマニュアルページ mbsegypsd(1)（英語）です。
