#include "contours.hpp"
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
int bruteForce(const cv::Mat& d, const cv::Mat& r) {
    std::vector<cv::Point> left, right; cv::findNonZero(d, left); cv::findNonZero(r, right);
    std::function<int(std::size_t, unsigned)> search = [&](std::size_t u, unsigned used) {
        if (u == left.size()) return 0;
        int best = search(u + 1, used);
        for (std::size_t v = 0; v < right.size(); ++v)
            if (!(used & (1u << v)) && std::abs(left[u].x - right[v].x) <= 1
                && std::abs(left[u].y - right[v].y) <= 1)
                best = std::max(best, 1 + search(u + 1, used | (1u << v)));
        return best;
    };
    return search(0, 0);
}
int main() {
    try {
        cv::Mat empty = cv::Mat::zeros(8, 8, CV_8U), d = empty.clone(), r = empty.clone();
        check(tp::evaluate(d, r, 1).metrics.performance == 1, "empty convention");
        d.at<uchar>(2, 2) = 255; r.at<uchar>(2, 3) = 255;
        check(tp::evaluate(d, r, 0).metrics.correct == 0, "exact shifted");
        check(tp::evaluate(d, r, 1).metrics.correct == 1, "tolerant shifted");
        d.at<uchar>(3, 3) = 255;
        auto m = tp::evaluate(d, r, 1).metrics;
        check(m.correct == 1 && m.fp == 1 && m.fn == 0, "no double count");
        check(std::abs(m.performance - 0.5) < 1e-12, "metric denominator");
        check(tp::evaluate(d, empty, 1).metrics.falsePositiveRate == 1, "empty reference");
        check(tp::evaluate(empty, r, 1).metrics.falseNegativeRate == 1, "empty detection");
        bool rejected = false;
        try { tp::evaluate(d, cv::Mat::zeros(3, 3, CV_8U), 1); } catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "size mismatch");
        std::mt19937 random(20261023);
        for (int trial = 0; trial < 150; ++trial) {
            cv::Mat a = cv::Mat::zeros(3, 3, CV_8U), b = a.clone();
            for (int i = 0; i < 9; ++i) { a.data[i] = random() % 3 == 0 ? 255 : 0; b.data[i] = random() % 3 == 0 ? 255 : 0; }
            auto e = tp::evaluate(a, b, 1);
            check(e.metrics.correct == bruteForce(a, b), "maximum matching vs exhaustive oracle");
            check(e.metrics.correct >= tp::evaluate(a, b, 0).metrics.correct, "tolerance monotonicity");
            check(e.metrics.correct == tp::evaluate(b, a, 1).metrics.correct, "symmetry");
            check(std::abs(e.metrics.performance + e.metrics.falsePositiveRate + e.metrics.falseNegativeRate - 1) < 1e-10, "P+TFP+TFN=1");
        }
        cv::Mat step(64, 64, CV_8U, cv::Scalar(0)); step.colRange(32, 64).setTo(255);
        {
            cv::Mat before = step.clone(); tp::detect(step, tp::Parameters());
            check(cv::countNonZero(before != step) == 0, "detect does not modify its input");
        }
        for (const auto& detector : {"sobel", "laplace", "canny"}) {
            tp::Parameters p; p.detector = detector; p.threshold = 10; p.low = 10; p.high = 20;
            for (int kernel : {3, 5, 7}) {
                p.kernel = kernel;
                auto edges = tp::detect(step, p);
                check(edges.type() == CV_8UC1 && edges.size() == step.size(), "binary output shape");
                check(cv::countNonZero(edges) > 0, "step edge detected");
                check(cv::countNonZero(tp::detect(cv::Mat(64, 64, CV_8U, cv::Scalar(128)), p)) == 0, "constant image has no edges");
            }
        }
        auto black = tp::binaryReference(cv::Mat(8, 8, CV_8U, cv::Scalar(255)), true);
        check(cv::countNonZero(black) == 0, "black edge polarity");
        std::cout << "OK : comptages, cas vides, tolerance, 150 graphes vs oracle exhaustif, 3 detecteurs et 3 noyaux.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
