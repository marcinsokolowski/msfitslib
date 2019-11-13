#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "datsrc.h"
#include "grsrc.h"



double LIBFUNC lsqgss(double const P_T *x, integer const P_T *nr,
               double const P_T *t)
{
    /* System generated locals */
    double ret_val, d__1, d__2;

    /* Local variables */
    double arg;

    /* Parameter adjustments */
    --x;

    /* Function Body */
/* Computing 2nd power */
    d__1 = x[2] - *t;
/* Computing 2nd power */
    d__2 = x[3];
    arg = d__1 * d__1 / (2. * (d__2 * d__2));
    if (arg > 700.) {
        ret_val = 0.;
    } else {
        ret_val = x[1] * exp(-arg);
    }
    return ret_val;
} /* lsqgss */


/* Common Block Declarations */

/*
struct {
    double c[1000], x2[1000], x1red[1000];
} dasv02_;

#define dasv02_1 dasv02_
*/

/* Common Block Declarations */

/*
struct {
    double c[1000], x2[1000], x1red[1000];
} dasv02_;

#define dasv02_1 dasv02_
*/

void LIBFUNC lsqnon_(double (CBFUNC P_T *userfn)(double const P_T *,
             integer const P_T *, double const P_T *), double const P_T *t,
             double const P_T *y, double const P_T *deltay,
             integer const P_T *n, integer const P_T *nr, integer P_T *nred,
             integer P_T *list, double P_T *x, double P_T *cx, double P_T *r,
             double P_T *a, double P_T *scrat, integer P_T *nstep)
{
    /* Table of constant values */

    integer c__1 = 1;
    double c_d0 = 0.;
    double c_dp5 = .5;

    /* System generated locals */
    integer a_dim1, a_offset, scrat_dim1, scrat_offset, cx_dim1, cx_offset,
            i__1, i__2, i__3;
    double d__1;

    /* Local variables */
    integer i, k, istep;
    double *c, *x2, *x1red, r1;
    logical ok, covmat;
    unsigned long amem;

    if ((amem = sizeof(double) * *n) > MAXALLOC) {
        MEMERROR("lsqnon_: c (too large)");
        goto f_0;
    }
    c = (double *)MEMALLOC(amem);
    if (!c) {
        MEMERROR("lsqnon_: c");
        goto f_0;
    }
    if ((amem = sizeof(double) * (*nr + *nred)) > MAXALLOC) {
        MEMERROR("lsqnon_: x2 (too large)");
        goto f_1;
    }
    x2 = (double *)MEMALLOC(amem);
    if (!x2) {
        MEMERROR("lsqnon_: x2");
        goto f_1;
    }
    x1red = x2 + *nr;

    /* Parameter adjustments */
    --deltay;
    --y;
    --t;
    --x;
    --list;
    scrat_dim1 = *nred;
    scrat_offset = scrat_dim1 + 1;
    scrat -= scrat_offset;
    a_dim1 = *n;
    a_offset = a_dim1 + 1;
    a -= a_offset;
    cx_dim1 = *nred;
    cx_offset = cx_dim1 + 1;
    cx -= cx_offset;

    /* Function Body */
    covmat = TRUE_;
    if (*nstep < 0) {
        covmat = FALSE_;
        *nstep = abs(*nstep);
    }
    if (*nstep < 1) {
        *nstep = 100;
    }
/* Initial value of minimum function */
    *r = 0.;
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
/* Computing 2nd power */
        d__1 = (y[i] - (*userfn)(&x[1], nr, &t[i])) / deltay[i];
        *r += d__1 * d__1;
    }
/* For NRED=0 : set NSTEP=1 and return */
    if (*nred <= 0) {
        *nstep = 1;
        goto L130;
    }
/* For NR=NRED :  set LIST */
    if (*nr == *nred) {
        i__1 = *nr;
        for (i = 1; i <= i__1; ++i) {
            list[i] = 1;
        }
    }
/* start iteration */
    i__1 = *nstep;
    for (istep = 1; istep <= i__1; ++istep) {
/* Numerical Derivatives */
        auxdri_(userfn, &x[1], &t[1], n, nr, nred, &list[1], &a[a_offset], &ok);
        if (! ok) {
            *nstep = -3;
            goto L130;
        }
        i__2 = *n;
        for (i = 1; i <= i__2; ++i) {
            i__3 = *nred;
            for (k = 1; k <= i__3; ++k) {
                a[i + k * a_dim1] /= deltay[i];
            }
        }
        i__2 = *n;
        for (i = 1; i <= i__2; ++i) {
            c[i - 1] = (y[i] - (*userfn)(&x[1], nr, &t[i])) / deltay[i];
        }
        mtxsvd_(&a[a_offset], c, x1red, &r1, n, nred, &c__1,&c_d0, &ok);
        if (! ok) {
            *nstep = -1;
            goto L130;
        }
        mtxzrv_(x2, nr);
        mtxpsv_(x2, x1red, nr, nred, &list[1]);
        mtxadv_(&x[1], x2, x2, nr);
        r1 = 0.;
        i__2 = *n;
        for (i = 1; i <= i__2; ++i) {
/* Computing 2nd power */
            d__1 = (y[i] - (*userfn)(x2, nr, &t[i])) / deltay[i];
            r1 += d__1 * d__1;
        }
        for (k = 1; k <= 30; ++k) {
            if (r1 > *r * 1.00000001 + 1e-15) {
/* minimum function has increased: reduce step by factor of two */
                mtxmsv_(x1red, x1red, &c_dp5, nred);
                mtxzrv_(x2, nr);
                mtxpsv_(x2, x1red, nr, nred, &list[1]);
                mtxadv_(&x[1], x2, x2, nr);
                r1 = 0.;
                i__2 = *n;
                for (i = 1; i <= i__2; ++i) {
/* Computing 2nd power */
                    d__1 = (y[i] - (*userfn)(x2, nr, &t[i])) / deltay[i];
                    r1 += d__1 * d__1;
                }
            } else {
                goto L90;
            }
        }
        *nstep = -4;
        goto L130;
L90:
        mtxzrv_(x2, nr);
        mtxpsv_(x2, x1red, nr, nred, &list[1]);
        mtxadv_(&x[1], x2, &x[1], nr);
/* test for break-off criterion */
        if ((d__1 = *r - r1, abs(d__1)) < abs(*r) * 1e-8 + 1e-15) {
            *r = r1;
/* Compute covariance matrix CX of unknowns */
            if (covmat) {
                auxdri_(userfn, &x[1], &t[1], n, nr, nred, &list[1],
                        &a[a_offset], &ok);
                if (! ok) {
                    *nstep = -3;
                    goto L130;
                }
                i__2 = *n;
                for (i = 1; i <= i__2; ++i) {
                    i__3 = *nred;
                    for (k = 1; k <= i__3; ++k) {
                        a[i + k * a_dim1] /= deltay[i];
                    }
                }
                mtxmat_(&a[a_offset], &a[a_offset], &cx[cx_offset], nred, n,
                        nred);
                mtxchi_(&cx[cx_offset], &scrat[scrat_offset], nred);
                *nstep = istep;
            }
            goto L130;
        } else {
            *r = r1;
        }
    }
    *nstep = -2;
L130:
    MEMFREE((void *)x2);
f_1:
    MEMFREE((void *)c);
f_0:
    return;
} /* lsqnon_ */


int FitGauss( double minValue, double& rms, double& center, double &max, int binNo, double binWidth, double* pCountTab, integer nstep=500)
{
	if(binNo>100) return(-5);
	static double T[100], Y[100], X[3], DELTAY[100], a[300], scrat[9], cx[9], r;
	static integer n=binNo, nr=3, nred=3, list[3];
	
	X[0]=max; X[1]=center; X[2]=rms;	// Initial fit values
	
	for(int i=0; i<binNo; i++)	// Generating histogram points
	{
		T[i] = minValue + i*binWidth + binWidth*0.5;
		Y[i] = pCountTab[i];
		DELTAY[i]=1;
	}
	
	lsqnon_(lsqgss_, T, Y, DELTAY, &n, &nr, &nred, list, X, cx, &r, a, scrat, &nstep);
	
	max=X[0]; center=X[1]; rms=X[2];	// Return values
	return nstep;
}