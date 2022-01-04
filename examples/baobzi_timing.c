#include <baobzi.h>

#include <math.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

double testfun_1d(const double *x) { return log(x[0]); }
double testfun_2d(const double *x) { return exp(cos(5.0 * x[0]) * sin(5.0 * x[1])); }
double testfun_2d_2(const double *x) { return exp(x[0] + 2 * sin(x[1])) * (x[0] * x[0] + log(2 + x[1])); }
double testfun_3d(const double *x) { return exp(x[0] + 2 * sin(x[1])) * (x[0] * x[0] + log(2 + x[1] * x[2])); }

void time_function(const baobzi_t *function, const double *x, int size, int n_runs) {
    const double time = omp_get_wtime();
    double res = 0.0;
    for (int i_run = 0; i_run < n_runs; ++i_run) {
        for (int i = 0; i < size; i += function->DIM) {
            res += baobzi_eval(function, x);
        }
    }
    const double dt = omp_get_wtime() - time;
    const long n_eval = n_runs * (size / function->DIM);
    printf("%g %g %g\n", dt, n_eval / (dt * 1E6), res);
}

void print_error(const baobzi_t *function, const double *x, int size) {
    double max_error = 0.0;
    double max_rel_error = 0.0;
    double mean_error = 0.0;
    double mean_rel_error = 0.0;

    size_t n_meas = 0;
    for (int i = 0; i < size; i += function->DIM) {
        const double *point = &x[i];

        double actual = function->f_(point);
        double interp = baobzi_eval(function, point);
        double delta = actual - interp;

        max_error = fmax(max_error, fabs(delta));

        if (fabs(actual) > 1E-15) {
            double rel_error = fabs(interp / actual - 1.0);
            max_rel_error = fmax(max_rel_error, rel_error);
            mean_rel_error += fabs(rel_error);
            n_meas++;
        }

        mean_error += fabs(delta);
    }
    mean_error = mean_error / n_meas;
    mean_rel_error = mean_rel_error / n_meas;

    printf("rel error max, mean: %g %g\n", max_rel_error, mean_rel_error);
    printf("abs error max, mean: %g %g\n", max_error, mean_error);
}

void test_func(double (*fin)(const double *), int dim, int order, const double *xin, const double *hl,
               const double *center, int n_points, int n_runs) {
    double *x_transformed = (double *)aligned_alloc(32, n_points * dim * sizeof(double));

    for (int i = 0; i < dim * n_points; i += dim)
        for (int j = 0; j < dim; ++j)
            x_transformed[i + j] = hl[j] * (2.0 * xin[i + j] - 1.0) + center[j];

    baobzi_t func_approx = baobzi_init(&testfun_2d, dim, order, center, hl, 1E-8);

    time_function(&func_approx, x_transformed, n_points * dim, n_runs);
    print_error(&func_approx, x_transformed, n_points * dim);

    free(x_transformed);
    baobzi_free(&func_approx);
}

int main(int argc, char *argv[]) {
    srand(1);
    size_t n_points = 1000000;
    size_t n_runs = 50;

    if (argc == 2)
        n_runs = atoi(argv[1]);

    double *x = (double *)aligned_alloc(32, n_points * 3 * sizeof(double));
    for (size_t i = 0; i < n_points * 3; ++i)
        x[i] = ((double)rand()) / RAND_MAX;

    {
        const int dim = 2;
        const int order = 6;
        double hl[2] = {1.0, 1.0};
        double center[2] = {hl[0] + 0.5, hl[1] + 2.0};
        test_func(&testfun_2d, dim, order, x, hl, center, n_points, n_runs);
    }

    {
        const int dim = 3;
        const int order = 6;
        double hl[3] = {1.0, 1.0, 1.0};
        double center[3] = {hl[0] + 0.5, hl[1] + 2.0, hl[2] + 0.5};
        test_func(&testfun_3d, dim, order, x, hl, center, n_points, n_runs);
    }

    return 0;
}