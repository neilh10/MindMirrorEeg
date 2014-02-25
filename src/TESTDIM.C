/* testdim.c
 * 
 * http://www.biomonitors.com/
 * Copyright (c) 1992-2014 Neil Hancock
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * 
Test file for dim.c
  DimEnterSnap
  DimRtrvSnap

*/
#define SIMULATING
#include "general.h"
#include "dim.h"

/* Proto */
void fill_data(int);

int main()
{
	int lp1,lp2;
	for (lp1 = 0; lp1 < 5; lp1++) {
		fill_data(lp1);
		DimEnterSnap(DIM_SNAP);
	}

	fill_data(++lp1);
	DimEnterSnap(DIM_START);

	for (lp2 = lp1; lp2 < 100; lp2++) {
		fill_data(lp2);
		DimEnterSnap(DIM_SNAP);
	}
 return 0;
}
extern Dimt_data dimu_input_data[];

void fill_data(int sample) {

	 dimu_input_data[0].lobe_data[LF_LOBE].input = 2*sample+1;
	 dimu_input_data[0].lobe_data[RF_LOBE].input = (3*sample)/2+1;
 }
 int ad_result;
 hw_att_value(){}
