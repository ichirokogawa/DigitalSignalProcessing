# DigitalSignalProcessing

[![Build VC++](https://github.com/ichirokogawa/DigitalSignalProcessing/actions/workflows/build.yml/badge.svg)](https://github.com/ichirokogawa/DigitalSignalProcessing/actions/workflows/build.yml)

デジタル信号処理の基礎演習として、C++20 と FFTW を使って5つの課題を実装しました。プログラム内で入力信号を生成し、波形、周波数スペクトル、フィルタ処理とヒルベルト変換の結果を CSV に出力します。

## 課題と結果

結果をグラフで確認するには、同梱の Excel ブックを開いてください。6枚のシートに、波形やスペクトルを示す12個のグラフを収録しています。[結果のグラフ（results.xlsx）](DigitalSignalProcessing/results.xlsx)

各課題では1秒間に100点を等間隔でサンプリングします。スペクトルの周波数間隔は 1 Hz です。

| 課題 | 確認できること | 出力データ |
| --- | --- | --- |
| 1. 余弦波の生成 | 振幅1、周波数 5 Hz の余弦波が、1秒間に5周期現れる。 | [余弦波](DigitalSignalProcessing/task1_cosine.csv) |
| 2. 余弦波の FFT | 片側スペクトルの 5 Hz 成分は振幅が1、位相がほぼ 0 rad。FFT の実部と虚部も確認できる。 | [余弦波のスペクトル](DigitalSignalProcessing/task2_cosine_fft.csv) |
| 3. 正弦波の FFT | 5 Hz の正弦波も振幅は1。課題2の余弦波と比べると、同じ周波数成分の位相が −π/2 rad ずれている。 | [正弦波](DigitalSignalProcessing/task3_sine.csv)・[スペクトル](DigitalSignalProcessing/task3_sine_fft.csv) |
| 4. ローパスフィルタ | 5 Hz（振幅1）と 30 Hz（振幅0.5）の余弦波を重ねる。遮断周波数を 10 Hz にすると、30 Hz 成分が除去され、5 Hz の余弦波が残る。 | [フィルタ適用前後の波形](DigitalSignalProcessing/task4_filter.csv) |
| 5. ヒルベルト変換 | 5 Hz の余弦波から解析信号を求める。実部は元の余弦波と重なり、虚部は位相が90°遅れた正弦波になる。 | [入力波形と解析信号](DigitalSignalProcessing/task5_hilbert.csv) |

## 実装上の工夫

- **FFTW の導入を NuGet にまとめました。** 演習資料では、FFTW を展開し、Visual Studio でインクルード先とリンク設定を指定してから DLL を手動でコピーします。このプロジェクトでは、自作の NuGet パッケージ `zyc.fftw.native` 3.3.11 を使用します。パッケージを復元するとヘッダーとライブラリの設定が適用され、ビルド時に必要な DLL が実行ファイルと同じディレクトリにコピーされます。[NuGet パッケージ](https://www.nuget.org/packages/zyc.fftw.native/3.3.11)
- **入力信号を C++ で生成します。** 演習資料で Excel が担っていた入力データの作成をプログラムに組み込みました。各サンプルの時刻を `x = i / N` とし、信号の値を FFTW の入力バッファに直接書き込みます。周波数や振幅を変えても、再ビルドして実行すれば、新しい条件で6個の CSV が得られます。[実装コード](DigitalSignalProcessing/DigitalSignalProcessing.cpp)

## ビルドと実行

Visual Studio 2026 の「C++ によるデスクトップ開発」ワークロードを使用します。FFTW パッケージは x64 用です。Developer PowerShell for Visual Studio を開き、リポジトリのルートで以下のコマンドを実行してください。

```powershell
msbuild DigitalSignalProcessing.slnx -restore -p:RestorePackagesConfig=true -p:Configuration=Release -p:Platform=x64
.\x64\Release\DigitalSignalProcessing.exe
```

実行すると、カレントディレクトリに `task1_cosine.csv` から `task5_hilbert.csv` までの6ファイルが出力されます。

GitHub Actions でも、プッシュ時と手動実行時に `Release|x64` 構成をビルドします。[ビルドワークフロー](.github/workflows/build.yml)
