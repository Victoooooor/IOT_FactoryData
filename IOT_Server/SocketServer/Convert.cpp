#include "stdafx.h"
#include "Convert.h"
#define V_low	-2000
#define V_high	2000
#define M_low	165
#define M_high	1735

float slope = .0;
float offset = .0;

int Electro_Meter(int measured) {
	if (slope == .0) {
		slope = (V_high - V_low) / (float(M_high - M_low));
		offset = float(V_high) - slope * M_high;
	}
	return int(measured * slope + offset);
}