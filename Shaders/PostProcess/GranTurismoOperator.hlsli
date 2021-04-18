#ifndef GRAN_TURISMO_OPERATOR_HLSLI
#define GRAN_TURISMO_OPERATOR_HLSLI

// http://cdn2.gran-turismo.com/data/www/pdi_publications/PracticalHDRandWCGinGTS_20181222.pdf
// https://github.com/tizian/tonemapper
// https://www.desmos.com/calculator/gslcdxvipg

struct GranTurismoOperator
{
	float MaximumBrightness;
	float Contrast;
	float LinearSectionStart;
	float LinearSectionLength;
	float BlackTightness_C;
	float BlackTightness_B;
};

float3 ApplyGranTurismoOperator(float3 x, GranTurismoOperator GTOperator)
{
	float3 P = (float3) GTOperator.MaximumBrightness;
	float3 a = (float3) GTOperator.Contrast;
	float3 m = (float3) GTOperator.LinearSectionStart;
	float3 l = (float3) GTOperator.LinearSectionLength;
	float3 c = (float3) GTOperator.BlackTightness_C;
	float3 b = (float3) GTOperator.BlackTightness_B;

    // Linear Region Computation
    // l0 is the linear length after scale
	float3 l0 = ((P - m) * l) / a;
	float3 L0 = m - (m / a);
	float3 L1 = m + (1.0.xxx - m) / a;
	float3 Lx = m + a * (x - m);

    // Toe
	float3 Tx = m * pow(x / m, c) + b;

    // Shoulder
	float3 S0 = m + l0;
	float3 S1 = m + a * l0;
	float3 C2 = (a * P) / (P - S1);
	float3 Sx = P - (P - S1) * exp(-(C2 * (x - S0) / P));

    // Toe weight
	float3 w0 = 1.0.xxx - smoothstep(0.0.xxx, m, x);
    // Shoulder weight
	float3 w2 = smoothstep(m + l0, m + l0, x);
    // Linear weight
	float3 w1 = 1.0.xxx - w0 - w2;

	return Tx * w0 + Lx * w1 + Sx * w2;
}

#endif