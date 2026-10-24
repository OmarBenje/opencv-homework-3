#pragma once
#include <opencv2/core.hpp>
#include <cstdint>
#include <string>

namespace tp {
struct Parameters {
    std::string detector = "canny";
    double sigma = 1.0;
    int kernel = 3;
    double threshold = 80.0;
    double low = 40.0;
    double high = 120.0;
};
struct Metrics {
    std::int64_t detected = 0, reference = 0, correct = 0, fp = 0, fn = 0;
    double performance = 0, falsePositiveRate = 0, falseNegativeRate = 0;
};
struct Evaluation {
    Metrics metrics;
    cv::Mat matchedDetected, matchedReference;
};
void validate(const Parameters& parameters);
cv::Mat detect(const cv::Mat& gray, const Parameters& parameters);
cv::Mat binaryReference(const cv::Mat& gray, bool blackEdges = false);
Evaluation evaluate(const cv::Mat& detected, const cv::Mat& reference, int radius);
cv::Mat overlay(const Evaluation& evaluation, const cv::Mat& detected,
                const cv::Mat& reference);
}
