/**
 * git clone https://github.com/hou-dao/deom.git
 * ---
 * Written by Houdao Zhang 
 * mailto: houdao@connect.ust.hk
 */
#include "deom.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

static void td_polar(deom &d, const double dt, const double ti, const double tf, const int nk, const mat &sdip, const cube &pdip, const vec &bdip, pulse &p) {

  const int nsys = d.nsys;
  const int nt_i = ceil(abs(ti) / dt);
  const int nt_f = ceil(abs(tf) / dt);
  const int nt = nt_i + nt_f;

  double t = -(nt_i - 1) * dt;
  cx_cube rho1 = zeros<cx_cube>(nsys, nsys, d.nmax);
  rho1(0, 0, 0) = 1.0;
  p.turnon();

  // rho propagation from ti to tf
  FILE *frho = fopen("td-rho.dat", "w");
  FILE *fpol = fopen("td-polar.dat", "w");
  for (int it = 0; it < nt; ++it) {

    fprintf(frho, "%16.6e", t / deom_fs2unit);
    for (int i = 0; i < nsys; ++i) {
      fprintf(frho, "%16.6e", real(rho1(i, i, 0)));
    }
    fprintf(frho, "%16.6e%16.6e", real(rho1(0, 1, 0)), imag(rho1(0, 1, 0)));
    fprintf(frho, "\n");

    cx_double polar = d.Trace(sdip, pdip, bdip, rho1);
    fprintf(fpol, "%16.6e%16.6e%16.6e\n", t / deom_fs2unit, real(polar), imag(polar));

    if (it % nk == 0) {
      printf("%s: %5.1f%%, nddo = %6d, lddo = %2d\n", "td-proprho",
             100 * it / static_cast<double>(nt), d.nddo, d.lddo);
    }
    d.rk4(rho1, t, dt, sdip, pdip, bdip, p);
    t += dt;
  }
  fclose(fpol);
  fclose(frho);
}

int main() {

  ifstream jsonFile("input.json");
  stringstream strStream;
  strStream << jsonFile.rdbuf();
  string jsonStr = strStream.str();
  string err;

  const Json json = Json::parse(jsonStr, err);
  if (!err.empty()) {
    printf("Error in parsing input file: %s\n", err.c_str());
    return 0;
  }

  deom d(json["deom"]);

  const double dt = json["td-rhot"]["dt"].number_value();
  const double ti = json["td-rhot"]["ti"].number_value();
  const double tf = json["td-rhot"]["tf"].number_value();
  const int nk = json["td-rhot"]["nk"].int_value();
  const string sdipFile = json["td-rhot"]["sdipFile"].string_value();
  const string pdipFile = json["td-rhot"]["pdipFile"].string_value();
  const string bdipFile = json["td-rhot"]["bdipFile"].string_value();

  pulse p(json["td-rhot"]["pulse"]);

  mat sdip;
  cube pdip;
  vec bdip;

  if (sdip.load(sdipFile, arma_ascii)) {
    sdip.print(sdipFile);
  } else {
    printf("Fail to load sdip");
  }

  if (pdip.load(pdipFile, arma_ascii)) {
    pdip.print(pdipFile);
  } else {
    printf("Fail to load pdip");
  }

  if (bdip.load(bdipFile, arma_ascii)) {
    bdip.print(bdipFile);
  } else {
    printf("Fail to load bdip");
  }

  td_polar(d, dt, ti, tf, nk, sdip, pdip, bdip, p);

  return 0;
}
