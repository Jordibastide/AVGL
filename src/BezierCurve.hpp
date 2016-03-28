#ifndef BEZIER_CURVE_H
#define BEZIER_CURVE_H

#include <vector>
#include <glm.hpp>

using namespace std;
using namespace glm;

class BezierCurve
{
    static const int g_FactorialMax = 33;
    double FactorialLookup[g_FactorialMax];

    public:
        BezierCurve();
        // void Bezier2D(double b[], int cpts, double p[]);
        vector<vec2> Bezier2D(vector<vec2> b, int cpts);
        vector<vec3> Bezier3D(vector<vec3> b, int cpts);
        int getFactorialMax();
        vector<vec3> InterpolateBetweenPoints(vector<vec3> b, int nbPointsBetween);

    private:
        double factorial(int n);
        void CreateFactorialTable();
        double Ni(int n, int i);
        double Bernstein(int n, int i, double t);
};

#endif