#include <stdint.h>
#include <math.h>

#include "debug.h"
#include "goertzel.h"

void goertzel_init(GOERTZEL_STATE *gp, uint32_t N, double k) {
        gp->k = k;
        gp->N = N;
        gp->A = 2 * M_PI * k / N;
        gp->B = 2 * cos(gp->A);
        gp->s0 = gp->s1 = gp->s2 = 0;
}

void goertzel_step(GOERTZEL_STATE *gp, double x) {
	gp->s0 = x + gp->B * gp->s1 - gp->s2;
	gp->s2 = gp->s1;
	gp->s1 = gp->s0;
}

double goertzel_strength(GOERTZEL_STATE *gp, double x) {
        // s0 = x[N-1] + B * s1 - s2
        gp->s0 = x + gp->B * gp->s1 - gp->s2;

        // C = exp (-j * A) = cos A - j sin A
        double re_C, im_C, re_D, im_D;
        re_C = gp->B / 2;    // Note: B = 2 * cos(A), so no need to recompute cos(A).
        im_C = -sin(gp->A);

        // D = exp (-j * 2 * pi * k * (N - 1) / N)
        double re_y, im_y;
        double d = 2 * M_PI * gp->k * (gp->N - 1) / gp->N;
        re_D = cos(d);
        im_D = -sin(d);

        // y = s0 - s1 * C
        re_y = gp->s0 - gp->s1 * re_C;
        im_y = -gp->s1 * im_C;

        // y = y * D
        double ry = re_y * re_D - im_y * im_D;
        im_y = im_y * re_D + re_y * im_D;
        re_y = ry;

	return 2 * (re_y * re_y + im_y * im_y) / (gp->N * gp->N);
}
