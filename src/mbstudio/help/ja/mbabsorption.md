# mbabsorption — 海水中の音波吸収（dB/km）を計算

`mbabsorption` は、周波数、水温、塩分、深度、pH、音速の関数として、海水中の音波吸収を dB/km 単位で計算します。ソナーの収録設定に入力する吸収係数を求める場合や、特定のソナー周波数と水塊について後方散乱強度の補正に使う値を確認する場合に使用します。

## 書式

    mbabsorption
        --depth=depth {-Ddepth}
        --frequency=frequency {-Ffrequency}
        --help {-H}
        --ph=ph {-Pph}
        --salinity=salinity {-Ssalinity}
        --sound-speed=soundspeed {-Csoundspeed}
        --temperature=temperature {-Ttemperature}
        --verbose {-V}

## 説明

計算には Francois と Garrison（1982）の式を使用します。この式は吸収を、ホウ酸、硫酸マグネシウム、純水の 3 つの寄与の和としてモデル化します。ホウ酸の寄与は約 10 kHz 以下で大きく、一般的なマルチビームの周波数では硫酸マグネシウムと純水の項が支配的です。

すべての入力には既定値があるため、異なる値だけを指定すれば十分です：周波数 200 kHz、水温 10 ℃、塩分 35 パーミル、深度 0 m（海面）、pH 8。音速を指定しない場合は、水温、塩分、深度から約 1412 + 3.21 T + 1.19 S + 0.0167 D（m/s）として推定されます。

通常は吸収の値（dB/km）だけを出力します。`-V` を指定すると、入力パラメータ、吸収値に加えて、塩分・水温・圧力から計算した海水密度（kg/m3）も表示します。

## 主なオプション

- `-Ffrequency` — 音波の周波数（kHz）（ロング形式 `--frequency=frequency`）。既定値：200。
- `-Ttemperature` — 水温（℃）（ロング形式 `--temperature=temperature`）。既定値：10.0。
- `-Ssalinity` — 塩分（パーミル）（ロング形式 `--salinity=salinity`）。既定値：35。
- `-Ddepth` — 吸収を計算する深度（m）（ロング形式 `--depth=depth`）。既定値：0.0。
- `-Pph` — 海水の pH（ロング形式 `--ph=ph`）。既定値：8。
- `-Csoundspeed` — 水中音速（m/s）（ロング形式 `--sound-speed=soundspeed`）。指定しない場合は水温、塩分、深度から計算します。
- `-V` — 詳細モード（ロング形式 `--verbose`）。使用したパラメータ、単位付きの結果、海水密度を表示します。
- `-H` — 簡単な説明と使い方を表示して終了します（ロング形式 `--help`）。

## 使用例

塩分 35 パーミル、pH 8、水温 4 ℃ の水中で、深度 1000 m（AUV 搭載など）で動作する 200 kHz マルチビームの吸収を計算します（約 `39.77` と表示されます）：

    mbabsorption -D1000 -F200 -P8 -S35 -T4

同じ計算を、パラメータと結果を読みやすく一覧表示して行います：

    mbabsorption -D1000 -F200 -P8 -S35 -T4 -V

温暖な熱帯海域の海面で、12 kHz の深海用マルチビームについて、実測音速 1540 m/s を使って吸収を計算します：

    mbabsorption -F12 -T27 -S35 -C1540

## 注意

- 単位：周波数は kHz（Hz ではありません）、深度はメートル、水温は ℃、塩分はパーミル（実用塩分）、音速は m/s です。
- オプションは小文字（`-f`、`-t`、`-s`、`-d`、`-p`、`-c`、`-v`、`-h`）でも指定できます。

## 関連項目

`mblevitus`、`mbbackangle`、`mbsvplist`。完全なリファレンスは MB-System のマニュアルページ mbabsorption(1)（英語）です。
