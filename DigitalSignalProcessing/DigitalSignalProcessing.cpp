
#define _USE_MATH_DEFINES

#include <fftw3.h>
#include <math.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <algorithm>

#define N     100
#define A     1.0
#define B     0.5
#define ALPHA 5
#define BETA  30

constexpr int LOW_PASS_CUTOFF = 10;

constexpr int HIGH_PASS_CUTOFF = 10;

// x = i / N は標本時刻（秒）。区間 [0, 1) の1秒間を N 点でサンプリングする
double GetX(int i)
{
	return static_cast<double>(i) / N;
}

double Cosine(double x)
{
	return A * cos(2 * M_PI * ALPHA * x);
}

double Sine(double x)
{
	return A * sin(2 * M_PI * ALPHA * x);
}

double Mixed(double x)
{
	return A * cos(2 * M_PI * ALPHA * x)
		+ B * cos(2 * M_PI * BETA * x);
}

// 信号のサンプルを FFTW の入力バッファに書き込む
void GenerateInput(fftw_complex* in, double (*signal)(double))
{
	for (int i = 0; i < N; i++)
	{
		in[i][0] = signal(GetX(i));
		in[i][1] = 0.0;
	}
}

std::ofstream OpenCsv(const char* filename)
{
	std::ofstream csv(filename);

	if (!csv)
	{
		throw std::runtime_error(std::string("Cannot open: ") + filename);
	}
		
	csv << std::setprecision(15);
	return csv;
}

// 課題1・3：元の波形を出力する
void ExportSignal(const char* filename, const fftw_complex* in)
{
	auto csv = OpenCsv(filename);

	csv << "X,Value\n";

	for (int i = 0; i < N; i++)
	{
		csv << GetX(i) << ","
			<< in[i][0] << "\n";
	}
}

// 課題2・3：FFT の結果から片側スペクトルを出力する
void ExportSpectrum(const char* filename, const fftw_complex* out)
{
	auto csv = OpenCsv(filename);

	csv << "Frequency,Real,Imaginary,Amplitude,Phase\n";

	for (int k = 0; k <= N / 2; k++)
	{
		double re = out[k][0];
		double im = out[k][1];

		double magnitude = hypot(re, im);

		// 片側振幅スペクトルを求めるため、振幅を正規化する
		double amplitude = magnitude / N;

		if (k != 0 && !(N % 2 == 0 && k == N / 2))
		{
			amplitude *= 2.0;
		}

		// 観測時間が1秒なので、周波数分解能は 1 Hz
		double frequency = static_cast<double>(k);

		csv << frequency << ","
			<< re << ","
			<< im << ","
			<< amplitude << ",";

		// 振幅がほぼゼロのときは位相を出力しない
		if (magnitude > 1e-9)
		{
			csv << atan2(im, re);
		}

		csv << "\n";
	}
}

// 課題4：理想ローパスフィルタ
void ApplyLowPassFilter(fftw_complex* spectrum)
{
	for (int k = 0; k < N; k++)
	{
		// 正と負の周波数成分に同じカットオフ周波数を適用する
		int frequency = std::min(k, N - k);

		if (frequency > LOW_PASS_CUTOFF)
		{
			spectrum[k][0] = 0.0;
			spectrum[k][1] = 0.0;
		}
	}
}

// 課題4.5：理想ハイパスフィルタ
void ApplyHighPassFilter(fftw_complex* spectrum)
{
	for (int k = 0; k < N; k++)
	{
		// 正と負の周波数成分に同じカットオフ周波数を適用する
		int frequency = std::min(k, N - k);

		if (frequency <= HIGH_PASS_CUTOFF)
		{
			spectrum[k][0] = 0.0;
			spectrum[k][1] = 0.0;
		}
	}
}




// 課題4：フィルタ適用前後の波形を出力する
void ExportFiltered(
	const char* filename,
	const fftw_complex* original,
	const fftw_complex* filtered)
{
	auto csv = OpenCsv(filename);

	csv << "X,Original,Filtered\n";

	for (int i = 0; i < N; i++)
	{
		csv << GetX(i) << ","
			<< original[i][0] << ","
			<< filtered[i][0] / N << "\n";
	}
}

// 課題5：ヒルベルト変換を用いて解析信号のスペクトルを求める
void ApplyHilbertTransform(fftw_complex* spectrum)
{
	for (int k = 0; k < N; k++)
	{
		// 直流成分はそのまま残す。サンプル数が偶数ならナイキスト周波数の成分も残す
		if (k == 0 || (N % 2 == 0 && k == N / 2)) 
		{
			continue;
		}

		if (k <= (N - 1) / 2)
		{
			// 正の周波数成分を2倍にする
			spectrum[k][0] *= 2.0;
			spectrum[k][1] *= 2.0;
		}
		else
		{
			// 負の周波数成分をゼロにする
			spectrum[k][0] = 0.0;
			spectrum[k][1] = 0.0;
		}
	}
}

// 課題5：解析信号の実部と虚部を出力する
void ExportHilbert(
	const char* filename,
	const fftw_complex* original,
	const fftw_complex* transformed)
{
	auto csv = OpenCsv(filename);

	csv << "X,Original,Real,Imaginary\n";

	for (int i = 0; i < N; i++)
	{
		csv << GetX(i) << ","
			<< original[i][0] << ","
			<< transformed[i][0] / N << ","
			<< transformed[i][1] / N << "\n";
	}
}

int main()
{
	fftw_complex* in = fftw_alloc_complex(N);
	fftw_complex* out = fftw_alloc_complex(N);
	fftw_complex* inverse = fftw_alloc_complex(N);

	if (!in || !out || !inverse)
	{
		std::cerr << "FFTW memory allocation failed.\n";
		fftw_free(in);
		fftw_free(out);
		fftw_free(inverse);
		return 1;
	}

	fftw_plan forwardPlan = fftw_plan_dft_1d(
		N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

	fftw_plan backwardPlan = fftw_plan_dft_1d(
		N, out, inverse, FFTW_BACKWARD, FFTW_ESTIMATE);

	if (!forwardPlan || !backwardPlan)
	{
		std::cerr << "FFTW plan creation failed.\n";

		if (forwardPlan) 
		{
			fftw_destroy_plan(forwardPlan);
		}

		if (backwardPlan)
		{
			fftw_destroy_plan(backwardPlan);
		}

		fftw_free(in);
		fftw_free(out);
		fftw_free(inverse);
		return 1;
	}

	bool success = true;

	try
	{
		// ================================
		// 課題1：余弦波の生成
		// ================================

		GenerateInput(in, Cosine);
		ExportSignal("task1_cosine.csv", in);

		// ================================
		// 課題2：余弦波の FFT
		// ================================

		fftw_execute(forwardPlan);
		ExportSpectrum("task2_cosine_fft.csv", out);


		// ================================
		// 課題3：正弦波の FFT
		// ================================

		GenerateInput(in, Sine);
		ExportSignal("task3_sine.csv", in);

		fftw_execute(forwardPlan);
		ExportSpectrum("task3_sine_fft.csv", out);


		// ================================
		// 課題4：ローパスフィルタの適用
		// ================================

		GenerateInput(in, Mixed);
		fftw_execute(forwardPlan);
		ApplyLowPassFilter(out);

		fftw_execute(backwardPlan);
		ExportFiltered("task4_filter.csv", in, inverse);

		// ================================
		// 課題4.5：ハイパスフィルタの適用
		// ================================

		GenerateInput(in, Mixed);
		fftw_execute(forwardPlan);
		ApplyHighPassFilter(out);

		fftw_execute(backwardPlan);
		ExportFiltered("task4.5_filter.csv", in, inverse);


		// ================================
		// 課題5：ヒルベルト変換
		// ================================

		GenerateInput(in, Cosine);
		fftw_execute(forwardPlan);
		ApplyHilbertTransform(out);

		fftw_execute(backwardPlan);
		ExportHilbert("task5_hilbert.csv", in, inverse);

		std::cout << "All tasks completed.\n";
	}
	catch (const std::exception& ex)
	{
		std::cerr << ex.what() << "\n";
		success = false;
	}

	fftw_destroy_plan(forwardPlan);
	fftw_destroy_plan(backwardPlan);

	fftw_free(in);
	fftw_free(out);
	fftw_free(inverse);

	return success ? 0 : 1;
}