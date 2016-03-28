#include "BezierCurve.hpp"

#include <iostream>

#include <string>
#include <cmath>

BezierCurve::BezierCurve()
{
    CreateFactorialTable();
}

// just check if n is appropriate, then return the result
double BezierCurve::factorial(int n)
{
    if (n < 0) { throw string("n is less than 0"); }
    if (n > 32) { throw string("n is greater than 32"); }

    return FactorialLookup[n]; /* returns the value n! as a SUMORealing point number */
}

// create lookup table for fast factorial calculation
void BezierCurve::CreateFactorialTable()
{
    // fill until n=32. The rest is too high to represent
    double a[g_FactorialMax];
    a[0] = 1.0;
    a[1] = 1.0;
    a[2] = 2.0;
    a[3] = 6.0;
    a[4] = 24.0;
    a[5] = 120.0;
    a[6] = 720.0;
    a[7] = 5040.0;
    a[8] = 40320.0;
    a[9] = 362880.0;
    a[10] = 3628800.0;
    a[11] = 39916800.0;
    a[12] = 479001600.0;
    a[13] = 6227020800.0;
    a[14] = 87178291200.0;
    a[15] = 1307674368000.0;
    a[16] = 20922789888000.0;
    a[17] = 355687428096000.0;
    a[18] = 6402373705728000.0;
    a[19] = 121645100408832000.0;
    a[20] = 2432902008176640000.0;
    a[21] = 51090942171709440000.0;
    a[22] = 1124000727777607680000.0;
    a[23] = 25852016738884976640000.0;
    a[24] = 620448401733239439360000.0;
    a[25] = 15511210043330985984000000.0;
    a[26] = 403291461126605635584000000.0;
    a[27] = 10888869450418352160768000000.0;
    a[28] = 304888344611713860501504000000.0;
    a[29] = 8841761993739701954543616000000.0;
    a[30] = 265252859812191058636308480000000.0;
    a[31] = 8222838654177922817725562880000000.0;
    a[32] = 263130836933693530167218012160000000.0;



    for(int i = 0; i < g_FactorialMax; ++i)
        FactorialLookup[i] = a[i];
}

double BezierCurve::Ni(int n, int i)
{
    double ni;
    double a1 = factorial(n);
    double a2 = factorial(i);
    double a3 = factorial(n - i);
    ni =  a1/ (a2 * a3);
    return ni;
}

// Calculate Bernstein basis
double BezierCurve::Bernstein(int n, int i, double t)
{
    double basis;
    double ti; /* t^i */
    double tni; /* (1 - t)^i */

    /* Prevent problems with pow */

    if (t == 0.0 && i == 0)
        ti = 1.0;
    else
        ti = pow(t, i);

    if (n == i && t == 1.0)
        tni = 1.0;
    else
        tni = pow((1 - t), (n - i));

    //Bernstein basis
    basis = Ni(n, i) * ti * tni;
    return basis;
}

/*
void BezierCurve::Bezier2D(double b[], int cpts, double p[])
{
    int npts = (b.Length) / 2;
    int icount, jcount;
    double step, t;

    // Calculate points on curve

    icount = 0;
    t = 0;
    step = (double)1.0 / (cpts - 1);

    for (int i1 = 0; i1 != cpts; i1++)
    {
        if ((1.0 - t) < 5e-6)
            t = 1.0;

        jcount = 0;
        p[icount] = 0.0;
        p[icount + 1] = 0.0;
        for (int i = 0; i != npts; i++)
        {
            double basis = Bernstein(npts - 1, i, t);
            p[icount] += basis * b[jcount];
            p[icount + 1] += basis * b[jcount + 1];
            jcount = jcount +2;
        }

        icount += 2;
        t += step;
    }
}
 */

vector<vec2> BezierCurve::Bezier2D(vector<vec2> b, int cpts)
{
    int icount;
    double step, t;
    vector<vec2> p(cpts);

    // Calculate points on curve

    icount = 0;
    t = 0;
    step = (double)1.0 / (cpts - 1);

    for (int i1 = 0; i1 != cpts; i1++)
    {
        if ((1.0 - t) < 5e-6)
            t = 1.0;
        p[icount] = vec2(0.0, 0.0);
        for (unsigned int jcount = 0; jcount != b.size(); ++jcount)
        {
            float basis = Bernstein(b.size() - 1, jcount, t);
            p[icount] += basis * b[jcount];
        }

        ++ icount;
        t += step;
    }

    return p;
}

vector<vec3> BezierCurve::Bezier3D(vector<vec3> b, int cpts)
{
    int icount;
    double step, t;
    vector<vec3> p(cpts);

    // Calculate points on curve

    icount = 0;
    t = 0;
    step = (double)1.0 / (cpts - 1);

    for (int i1 = 0; i1 != cpts; i1++)
    {
        if ((1.0 - t) < 5e-6)
            t = 1.0;
        p[icount] = vec3(0.0, 0.0, 0.0);
        for (unsigned int jcount = 0; jcount != b.size(); ++jcount)
        {
            float basis = Bernstein(b.size() - 1, jcount, t);
            p[icount] += basis * b[jcount];
        }

        ++ icount;
        t += step;
    }

    return p;
}

int BezierCurve::getFactorialMax()
{
    return g_FactorialMax;
}

vector<vec3> BezierCurve::InterpolateBetweenPoints(vector<vec3> b, int nbPointsBetween)
{
    vector<vec3> p;
    p.push_back(b[0]);

    for(unsigned int i = 1; i < b.size(); ++i)
    {
        for(int j = 1; j < nbPointsBetween; ++j)
        {
            double ratio = double(j) / nbPointsBetween;
            double ratioNext = 1.0 - ratio;

            p.push_back(float(ratio) * b[i] + float(ratioNext) * b[i - 1]);
        }
    }

    return p;
}