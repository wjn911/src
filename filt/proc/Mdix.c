/* Convert RMS to interval velocity using LS and shaping regularization.

Takes: rect1= rect2= ...

rectN defines the size of the smoothing stencil in N-th dimension.
*/
/*
  Copyright (C) 2004 University of Texas at Austin
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <math.h>

#include <rsf.h>

#include "smoothder.h"

int main(int argc, char* argv[])
{
    int i, ncycle, niter, nd, dim, n1, n2, i1, i2;
    int n[SF_MAX_DIM], rect[SF_MAX_DIM];
    float **vr, **vi, **wt, **p=NULL, wti;
    char key[6];
    bool diff, dip;
    sf_file vrms, vint, weight, vout, slope;

    sf_init(argc,argv);
    vrms = sf_input("in");
    vint = sf_output("out");
    weight = sf_input("weight");

    dim = sf_filedims (vrms,n);

    for (i=0; i < dim; i++) {
	snprintf(key,6,"rect%d",i+1);
	if (!sf_getint(key,rect+i)) rect[i]=1;
    }

    if (!sf_getbool("diff",&diff)) diff=false;
    /* if y, apply anisotropic diffusion */

    if (!sf_getbool("dip",&dip)) dip=false;
    /* if y, apply directional shaping */

    nd = smoothder_init(dim, rect, n, diff, dip);
    n1 = n[0];
    n2 = nd/n1;

    if (dip) {
	slope = sf_input("slope");
	p = sf_floatalloc2(n1,n2);
	sf_floatread(p[0],nd,slope);
	sf_fileclose(slope);
    }
    
    vr = sf_floatalloc2(n1,n2);
    vi = sf_floatalloc2(n1,n2);
    wt = sf_floatalloc2(n1,n2);

    sf_floatread(vr[0],nd,vrms);
    sf_floatread(wt[0],nd,weight);

    if (!sf_getint("niter",&niter)) niter=100;
    /* maximum number of iterations */

    if (!sf_getint("ncycle",&ncycle)) ncycle=10;
    /* number of cycles for anisotropic diffusion */

    wti = 0.;
    for (i2=0; i2 < n2; i2++) {
	for (i1=0; i1 < n1; i1++) {
	    wti += wt[i2][i1]*wt[i2][i1];
	}
    }
    if (wti > 0.) wti = sqrtf(n1*n2/wti);

    for (i2=0; i2 < n2; i2++) {
	for (i1=0; i1 < n1; i1++) {
	    vr[i2][i1] *= vr[i2][i1]*(i1+1.);
	    wt[i2][i1] *= wti/(i1+1.); /* decrease weight with time */	    
	}
    }
    
    if (dip) {
	smoothdip(niter, p, wt[0], vr[0], vi[0]);
    } else if (diff) {
	smoothdiff(niter, ncycle, wt[0], vr[0], vi[0]);
    } else {
	smoothder(niter, wt[0], vr[0], vi[0]);
    }

    for (i2=0; i2 < n2; i2++) {
	for (i1=0; i1 < n1; i1++) {
	    vr[i2][i1] = sqrtf(vr[i2][i1]/(i1+1.));
	    vi[i2][i1] = sqrtf(vi[i2][i1]);
	}
    }

    sf_floatwrite(vi[0],nd,vint);

    if (NULL != sf_getstring("vrmsout")) {
	/* optionally, output predicted vrms */
	vout = sf_output("vrmsout");

	sf_floatwrite(vr[0],nd,vout);
    }

    exit(0);
}

/* 	$Id$	 */
