/*
C++ to GNUPlot (gnuplot_i.hpp) source code:
https://code.google.com/archive/p/gnuplot-cpp/

CSV reader (csv.h) source code:
https://github.com/ben-strasser/fast-cpp-csv-parser/

CSV writer (csv_writer.h) source code:
https://github.com/vincentlaucsb/csv-parser
*/

#include "graph_helper.hpp"
#include "lib/csv.h"
#include "lib/csv_writer.hpp"
#include <bits/stdc++.h>
using namespace std;

typedef tuple<double, double, double, double, double> ddddd;
typedef tuple<double, double> dd;
typedef vector<ddddd> vddddd;

template <class S, class T>
tuple<S, T> operator+(const tuple<S, T>& lhs, const tuple<S, T>& rhs) {
	return make_tuple(get<0>(lhs) + get<0>(rhs), get<1>(lhs) + get<1>(rhs));
}

template <class S, class T>
tuple<S, T> operator/(const tuple<S, T>& lhs, const double& rhs) {
	return make_tuple(get<0>(lhs) / rhs, get<1>(lhs) / rhs);
}

dd runEpoch(double& w1, double& w2, double& w3, double& w4, double& b, int bv, int ev, int fold, int maxFold, double lrate, int epochNum);
double train(int idx, double& w1, double& w2, double& w3, double& w4, double& b, double lrate);
double validation(double w1, double w2, double w3, double w4, double b, int bv, int ev, int maxFold);
void slpRun(double lrate, int maxFold, int maxEpoch);

vddddd csvdata;
int kFold = 5;
int epoch = 300;

int main() {
	double x1, x2, x3, x4, y;
	io::CSVReader<5> csv("iris-2-target.csv");
	csv.read_header(io::ignore_extra_column, "x1", "x2", "x3", "x4", "y");
	while (csv.read_row(x1, x2, x3, x4, y)) {
		csvdata.push_back(make_tuple(x1, x2, x3, x4, y));
	}

	slpRun(0.1, kFold, epoch);
	slpRun(0.8, kFold, epoch);

	getchar();
}

void slpRun(double lrate, int maxFold, int maxEpoch) {
	char cname[255];
	sprintf(cname, "%.1lf", lrate);
	string lrateName = string(cname);

	vector<dd> resultGraphData;
	for (int i = 0; i < maxEpoch; i++) {
		resultGraphData.push_back(make_tuple(0.0, 0.0));
	}

	double lower_bound = 0.01;
	double upper_bound = 1;
	uniform_real_distribution<double> unif(lower_bound, upper_bound);
	default_random_engine re;
	double iw1 = unif(re);
	double iw2 = unif(re);
	double iw3 = unif(re);
	double iw4 = unif(re);
	double ib = unif(re);

	for (int i = 0; i < maxFold; i++) {
		double w1, w2, w3, w4, b;
		w1 = iw1;
		w2 = iw2;
		w3 = iw3;
		w4 = iw4;
		b = ib;

		int beginv, endv;
		beginv = i * (csvdata.size() / maxFold);
		endv = beginv + (csvdata.size() / maxFold);

		for (int j = 0; j < maxEpoch; j++) {
			dd epochResult = runEpoch(w1, w2, w3, w4, b, beginv, endv, i, maxFold, lrate, j);
			resultGraphData[j] = resultGraphData[j] + (epochResult / (double)maxFold);
		}
	}

	vector<double> errx, erry, accx, accy;
	for (int i = 0; i < maxEpoch; i++) {
		errx.push_back(i);
		accx.push_back(i);
		erry.push_back(get<0>(resultGraphData[i]));
		accy.push_back(get<1>(resultGraphData[i]));
	}

	string errName = "error " + lrateName, errTitle = "Error/epoch (lr=" + lrateName + ")", errStyle = "lw 3", errColor = "#FF0000";
	buildGraph(errName, errx, erry, false, true, -10, maxEpoch + 10, 0, 0, errTitle, errStyle, errColor);

	string accName = "accuracy " + lrateName, accTitle = "Accuracy/epoch (lr=" + lrateName + ")", accStyle = "lw 3", accColor = "#0000FF";
	buildGraph(accName, accx, accy, false, false, -10, maxEpoch + 10, 0, 1.05, accTitle, accStyle, accColor);
}

dd runEpoch(double& w1, double& w2, double& w3, double& w4, double& b, int bv, int ev, int fold, int maxFold, double lrate, int epochNum) {
	double err = 0.0;
	double ew1, ew2, ew3, ew4, eb;
	ew1 = w1;
	ew2 = w2;
	ew3 = w3;
	ew4 = w4;
	eb = b;
	for (int i = 0; i < bv; i++) {
		err += train(i, ew1, ew2, ew3, ew4, eb, lrate);
	}
	for (int i = ev; i < csvdata.size(); i++) {
		err += train(i, ew1, ew2, ew3, ew4, eb, lrate);
	}
	double acc = validation(ew1, ew2, ew3, ew4, eb, bv, ev, maxFold);
	w1 = ew1;
	w2 = ew2;
	w3 = ew3;
	w4 = ew4;
	b = eb;

	//printf("fold %d, epoch %d -> err: %lf acc: %lf\n", fold, epochNum, err, acc);
	return make_tuple(err / (double(csvdata.size()) - double(csvdata.size() / maxFold)), acc);
}

double train(int idx, double& w1, double& w2, double& w3, double& w4, double& b, double lrate) {
	ddddd datai = csvdata[idx];
	double y = get<0>(datai) * w1 + get<1>(datai) * w2 + get<2>(datai) * w3 + get<3>(datai) * w4 + b;
	double g = (double)1.0 / (1.0 + exp(-y));
	double dw1 = 2.0 * (g - get<4>(datai)) * g * (1.0 - g) * get<0>(datai);
	double dw2 = 2.0 * (g - get<4>(datai)) * g * (1.0 - g) * get<1>(datai);
	double dw3 = 2.0 * (g - get<4>(datai)) * g * (1.0 - g) * get<2>(datai);
	double dw4 = 2.0 * (g - get<4>(datai)) * g * (1.0 - g) * get<3>(datai);
	double db = 2.0 * (g - get<4>(datai)) * g * (1.0 - g);
	w1 -= (lrate * dw1);
	w2 -= (lrate * dw2);
	w3 -= (lrate * dw3);
	w4 -= (lrate * dw4);
	b -= (lrate * db);
	double error = (get<4>(datai) - g) * (get<4>(datai) - g);
	//printf("data %d, activation: %lf error: %lf\n", idx, g, error);

	return error;
}

double validation(double w1, double w2, double w3, double w4, double b, int bv, int ev, int maxFold) {
	int tp, tn, fp, fn;
	tp = tn = fp = fn = 0;

	for (int i = bv; i < ev; i++) {
		ddddd datai = csvdata[i];
		double y = get<0>(datai) * w1 + get<1>(datai) * w2 + get<2>(datai) * w3 + get<3>(datai) * w4 + b;
		double g = (double)1 / (1 + exp(-y));
		double p = (g >= 0.5) ? 1.0 : 0.0;
		double t = get<4>(datai);
		if (p == 1 && t == 1)
			tp++;
		else if (p == 1 && t == 0)
			fp++;
		else if (p == 0 && t == 0)
			tn++;
		else if (p == 0 && t == 1)
			fn++;
	}

	return double(tp + tn) / double(csvdata.size() / maxFold);
}