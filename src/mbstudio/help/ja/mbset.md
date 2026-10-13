# mbset — mbprocess パラメータファイルの作成と変更

`mbset` は、`mbprocess` の動作を制御する `.par` パラメータファイルを作成・変更します。`mbprocess` が使用する SVP、航法、潮汐、編集ファイルや、適用する補正（ロールバイアス、喫水、ヘディングオフセットなど）を、1 つのスワスファイルまたはデータリスト内の全ファイルに対して一括で設定できます。

## 書式

    mbset
        --explicit {-E}
        --format=format_id {-Fformat_id}
        --help {-H}
        --input=file {-Ifile}
        --look-for-files {-L}
        --parameter=PARAMETER:value {-PPARAMETER:value}
        --remove-nav-adjust {-N}
        --verbose {-V}

## 説明

各スワスデータファイルは、同じ名前に `.par` を付けた `mbprocess` パラメータファイルを持つことができます（`survey.mb58` なら `survey.mb58.par`）。このファイルには `SVPFILE`、`ROLLBIAS`、`EDITSAVEFILE` などのコマンドが 1 行に 1 つずつ書かれています。`mbset` は既存のパラメータファイルを読み込み（ない場合は既定のパラメータから開始し）、`-P` で指定したパラメータを変更してファイルに書き戻します。

`-P` オプションは 1 つのパラメータを設定するもので、必要なだけ繰り返し指定できます。入力がデータリストの場合は、そこから（再帰的に）参照されるすべてのスワスファイルのパラメータファイルに同じ変更が適用されます。調査全体に統一した処理パラメータを設定する一般的な方法です。

`-E` を指定しない限り、ファイルや値を設定すると対応する処理モードも自動的にオンになります。たとえば `-PSVPFILE:best.svp` は SVP が実際に使われるよう `SVPMODE` も 1 に設定し、`-PROLLBIAS:0.5` は `ROLLBIASMODE` を 1 に、`-PDRAFT:1.95` は `DRAFTMODE` を 4（一定喫水）に設定します。`-PTIDEFILE:...` は潮汐補正をオンにし、`-PNAVADJFILE:...` は調整済み航法と深度オフセットのマージ（`NAVADJMODE` 2）をオンにします。

よく使うパラメータコマンド（完全な一覧と各モードの意味はマニュアルページを参照してください）:

- 航法: `NAVMODE`, `NAVFILE`, `NAVFORMAT`, `NAVHEADING`, `NAVSPEED`, `NAVDRAFT`, `NAVATTITUDE`, `NAVINTERP`, `NAVTIMESHIFT`, `NAVSHIFT`, `NAVOFFSETX`, `NAVOFFSETY`, `NAVSHIFTLON`, `NAVSHIFTLAT`, `NAVADJMODE`, `NAVADJFILE`。
- 姿勢とソナー深度: `ATTITUDEMODE`, `ATTITUDEFILE`, `ATTITUDEFORMAT`, `SONARDEPTHMODE`, `SONARDEPTHFILE`, `SONARDEPTHFORMAT`。
- 編集とデータカット: `EDITSAVEMODE`, `EDITSAVEFILE`, `DATACUTCLEAR`, `DATACUT`, `BATHCUTNUMBER`, `BATHCUTDISTANCE`, `BATHCUTSPEED`, `AMPCUTNUMBER`, `SSCUTNUMBER` など。
- 水深の再計算: `SVPMODE`, `SVPFILE`, `SSVMODE`, `SSV`, `TTMULTIPLY`, `ANGLEMODE`, `SOUNDSPEEDREF`, `STATICMODE`, `STATICFILE`。
- 補正: `DRAFTMODE`, `DRAFT`, `DRAFTOFFSET`, `DRAFTMULTIPLY`, `HEAVEMODE`, `HEAVEOFFSET`, `LEVERMODE`, `ROLLBIASMODE`, `ROLLBIAS`, `ROLLBIASPORT`, `ROLLBIASSTBD`, `PITCHBIASMODE`, `PITCHBIAS`, `HEADINGMODE`, `HEADINGOFFSET`, `TIDEMODE`, `TIDEFILE`, `TIDEFORMAT`。
- 振幅とサイドスキャン: `AMPCORRMODE`, `AMPCORRFILE`, `AMPCORRSYMMETRY`, `AMPCORRANGLE`, `AMPCORRSLOPE`, `SSCORRMODE`, `SSCORRFILE`, `SSCORRSYMMETRY`, `SSCORRANGLE`, `SSCORRSLOPE`, `AMPSSCORRTOPOFILE`, `SSRECALCMODE`。
- 一般とメタデータ: `FORMAT`, `OUTFILE`, `METAVESSEL`, `METAINSTITUTION`, `METAPLATFORM`, `METASONAR`, `METACRUISEID`, `METACRUISENAME`, `METAPI`, `METACLIENT` など。

## 主なオプション

- `-I<file>` / `--input=<file>` — パラメータファイルを作成・更新するスワスファイルまたはデータリスト。既定値: `datalist.mb-1`。
- `-P<PARAMETER>:<value>` / `--parameter=<PARAMETER>:<value>` — パラメータを 1 つ設定します。複数設定するには繰り返します。区切り文字は `:` または `=` で、最初の 1 つだけが区切りとして扱われるため、値自体に `:`（Windows のドライブ文字など）を含めることができます。
- `-E` / `--explicit` — 処理モードを自動的にオンにしません。すべてのモードを `-P` で明示的に設定する必要があります。
- `-L` / `--look-for-files` — 各スワスファイルと同じフォルダで標準の名前の編集保存ファイルや航法ファイルを探し、見つかればそれらを使うようパラメータを設定します。
- `-N` / `--remove-nav-adjust` — `mbnavadjust` の調整済み航法ファイルへの参照を削除します。参照先のファイルは削除され、`NAVADJMODE`／`NAVADJFILE` がリセットされます。
- `-F<format_id>` / `--format=<format_id>` — 入力の MBIO フォーマット ID。ファイル名からの自動判別より優先されます。
- `-V` / `--verbose` — 各パラメータファイルに書き込んだパラメータを一覧表示します。
- `-H` / `--help` — 簡単なヘルプを表示して終了します。

## 使用例

1 つのファイルに、SVP ファイル、+0.5 度のロールバイアス、1.95 m の一定喫水を設定します。

    mbset -I0051_20010829_223755.mb57 -PSVPFILE:0051_20010829_223755.mb57.svp -PROLLBIAS:0.5 -PDRAFT:1.95 -V

調査のすべてのファイルに同じ潮汐ファイル（フォーマット 2: 年 月 日 時 分 秒 潮位）を適用します。

    mbset -Idatalist.mb-1 -PTIDEFILE:survey.tide -PTIDEFORMAT:2

`mbgetesf` で抽出した編集保存ファイルを適用します。

    mbset -Ihs_ew9103_143.mb24 -PEDITSAVEMODE:1 -PEDITSAVEFILE:hs_ew9103_143.mb24.esf

その後、データを処理します。

    mbprocess -Idatalist.mb-1

## 注意

- `mbset` はパラメータファイルを書き込むだけです。`mbprocess` を実行するまで処理済みデータは変わりません。
- ファイル名の値は最初のスペースまでしか読み込まれないため、パラメータで指定するファイル名にスペースを含めることはできません。相対パスはスワスファイルのフォルダからの相対として扱われるため、通常は補助ファイルをデータと同じフォルダに置きます。
- パラメータ名は大文字で記述する必要があります。

## 関連項目

`mbprocess`, `mbclean`, `mbsvplist`, `mbbackangle`, `mbgetesf`, `mbotps`, `mbdatalist`。完全なリファレンスは MB-System のマニュアルページ mbset(1)（英語）です。
