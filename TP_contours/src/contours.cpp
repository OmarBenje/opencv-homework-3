#include "contours.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <vector>

namespace tp {
namespace {
void requireGray(const cv::Mat& image) {
    if (image.empty() || image.type() != CV_8UC1)
        throw std::invalid_argument("Image non vide en niveaux de gris 8 bits requise.");
}
Metrics counts(std::int64_t d, std::int64_t r, std::int64_t c) {
    Metrics m;
    m.detected = d; m.reference = r; m.correct = c;
    m.fp = d - c; m.fn = r - c;
    const auto denominator = c + m.fp + m.fn;
    // Deux ensembles vides sont identiques : P=1, TFP=TFN=0.
    if (denominator == 0) m.performance = 1;
    else {
        m.performance = static_cast<double>(c) / denominator;
        m.falsePositiveRate = static_cast<double>(m.fp) / denominator;
        m.falseNegativeRate = static_cast<double>(m.fn) / denominator;
    }
    return m;
}
}
void validate(const Parameters& p) {
    if (p.detector != "sobel" && p.detector != "laplace" && p.detector != "canny")
        throw std::invalid_argument("Detecteur attendu : sobel, laplace ou canny.");
    if (!std::isfinite(p.sigma) || p.sigma < 0 || p.sigma > 5)
        throw std::invalid_argument("Sigma doit etre compris entre 0 et 5.");
    if (p.kernel != 3 && p.kernel != 5 && p.kernel != 7)
        throw std::invalid_argument("Noyau attendu : 3, 5 ou 7.");
    if (!std::isfinite(p.threshold) || !std::isfinite(p.low) || !std::isfinite(p.high)
        || p.threshold < 0 || p.low < 0 || p.high < p.low)
        throw std::invalid_argument("Seuils invalides : 0 <= low <= high, threshold >= 0.");
}
cv::Mat binaryReference(const cv::Mat& gray, bool blackEdges) {
    requireGray(gray);
    cv::Mat binary;
    cv::threshold(gray, binary, 127, 255,
                  blackEdges ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY);
    return binary;
}
cv::Mat detect(const cv::Mat& gray, const Parameters& p) {
    requireGray(gray); validate(p);
    // Pas de `smooth = gray` : le flou ecrirait dans l'image source (buffer partage).
    cv::Mat smooth, response, binary;
    if (p.sigma == 0) smooth = gray;
    else {
        // Rayon de trois sigmas ; sigma=0 desactive explicitement le lissage.
        int size = 2 * static_cast<int>(std::ceil(3 * p.sigma)) + 1;
        cv::GaussianBlur(gray, smooth, cv::Size(size, size), p.sigma, p.sigma,
                         cv::BORDER_REFLECT_101);
    }
    if (p.detector == "canny") {
        cv::Canny(smooth, binary, p.low, p.high, p.kernel, true);
    } else {
        if (p.detector == "sobel") {
            cv::Mat gx, gy;
            cv::Sobel(smooth, gx, CV_32F, 1, 0, p.kernel, 1, 0, cv::BORDER_REFLECT_101);
            cv::Sobel(smooth, gy, CV_32F, 0, 1, p.kernel, 1, 0, cv::BORDER_REFLECT_101);
            cv::magnitude(gx, gy, response);
        } else {
            cv::Mat laplacian;
            cv::Laplacian(smooth, laplacian, CV_32F, p.kernel, 1, 0, cv::BORDER_REFLECT_101);
            response = cv::abs(laplacian);
        }
        // Seuillage de la reponse brute, sans normalisation par image ni saturation.
        cv::compare(response, p.threshold, binary, cv::CMP_GT);
    }
    return binary;
}
Evaluation evaluate(const cv::Mat& detected, const cv::Mat& reference, int radius) {
    requireGray(detected); requireGray(reference);
    if (detected.size() != reference.size())
        throw std::invalid_argument("Tailles image/reference differentes.");
    if (radius != 0 && radius != 1)
        throw std::invalid_argument("Tolerance attendue : 0 (exact) ou 1 (3x3).");
    Evaluation result;
    result.matchedDetected = cv::Mat::zeros(detected.size(), CV_8U);
    result.matchedReference = cv::Mat::zeros(reference.size(), CV_8U);
    std::vector<cv::Point> left, right;
    cv::findNonZero(detected, left); cv::findNonZero(reference, right);
    if (radius == 0) {
        cv::Mat d, r;
        cv::compare(detected, 0, d, cv::CMP_GT);
        cv::compare(reference, 0, r, cv::CMP_GT);
        cv::bitwise_and(d, r, result.matchedDetected);
        result.matchedReference = result.matchedDetected.clone();
        result.metrics = counts(left.size(), right.size(), cv::countNonZero(result.matchedDetected));
        return result;
    }
    // Graphe biparti : un pixel detecte peut correspondre a au plus 9 references.
    // Un appariement maximum (Hopcroft-Karp) interdit les doubles comptes.
    cv::Mat ids(reference.size(), CV_32S, cv::Scalar(-1));
    for (int v = 0; v < static_cast<int>(right.size()); ++v) ids.at<int>(right[v]) = v;
    std::vector<std::vector<int>> adjacency(left.size());
    for (std::size_t u = 0; u < left.size(); ++u) {
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
            const auto q = left[u] + cv::Point(dx, dy);
            if (q.x >= 0 && q.x < ids.cols && q.y >= 0 && q.y < ids.rows) {
                int v = ids.at<int>(q);
                if (v >= 0) adjacency[u].push_back(v);
            }
        }
    }
    std::vector<int> pairLeft(left.size(), -1), pairRight(right.size(), -1);
    std::vector<int> distance(left.size());
    const int infinity = std::numeric_limits<int>::max();
    int shortest = infinity;
    auto bfs = [&]() {
        std::queue<int> queue;
        shortest = infinity;
        for (int u = 0; u < static_cast<int>(left.size()); ++u) {
            distance[u] = pairLeft[u] < 0 ? 0 : infinity;
            if (pairLeft[u] < 0) queue.push(u);
        }
        while (!queue.empty()) {
            int u = queue.front(); queue.pop();
            if (distance[u] >= shortest) continue;
            for (int v : adjacency[u]) {
                int next = pairRight[v];
                if (next < 0) shortest = distance[u] + 1;
                else if (distance[next] == infinity) {
                    distance[next] = distance[u] + 1; queue.push(next);
                }
            }
        }
        return shortest != infinity;
    };
    std::function<bool(int)> dfs = [&](int u) {
        for (int v : adjacency[u]) {
            int next = pairRight[v];
            if ((next < 0 && distance[u] + 1 == shortest)
                || (next >= 0 && distance[next] == distance[u] + 1 && dfs(next))) {
                pairLeft[u] = v; pairRight[v] = u; return true;
            }
        }
        distance[u] = infinity; return false;
    };
    std::int64_t correct = 0;
    while (bfs()) for (int u = 0; u < static_cast<int>(left.size()); ++u)
        if (pairLeft[u] < 0 && dfs(u)) ++correct;
    for (std::size_t u = 0; u < left.size(); ++u) if (pairLeft[u] >= 0) {
        result.matchedDetected.at<uchar>(left[u]) = 255;
        result.matchedReference.at<uchar>(right[pairLeft[u]]) = 255;
    }
    result.metrics = counts(left.size(), right.size(), correct);
    return result;
}
cv::Mat overlay(const Evaluation& e, const cv::Mat& detected, const cv::Mat& reference) {
    cv::Mat display(detected.size(), CV_8UC3, cv::Scalar(15, 15, 15));
    for (int y = 0; y < display.rows; ++y) for (int x = 0; x < display.cols; ++x) {
        bool d = detected.at<uchar>(y, x) != 0;
        bool missed = reference.at<uchar>(y, x) && !e.matchedReference.at<uchar>(y, x);
        if (missed) display.at<cv::Vec3b>(y, x) = {255, 160, 40};
        if (d) display.at<cv::Vec3b>(y, x) = e.matchedDetected.at<uchar>(y, x)
            ? cv::Vec3b(80, 220, 80) : (missed ? cv::Vec3b(220, 60, 220) : cv::Vec3b(70, 70, 250));
    }
    return display;
}
}
