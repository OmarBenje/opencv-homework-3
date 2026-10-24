#include "contours.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;
struct Options {
    tp::Parameters parameters;
    fs::path images = "data/images", references = "data/images/gt";
    fs::path output = "results/manual", list;
    bool gui = true, saveImages = false, blackEdges = true, guiCheck = false;
    int tolerance = 1;
};
struct Sample { std::string name; cv::Mat image, reference; };
void help() {
    std::cout << R"(TP_contours - Sobel, Laplace et Canny (C++ / OpenCV)
Depuis la racine du projet : ./build/contours [options]
  --gui                  Fenetre interactive avec curseurs (defaut)
  --gui-check            Test automatique des curseurs et export de 3 vues
  --batch                Traitement du lot sans interface
  --images DOSSIER       Images sources (defaut data/images)
  --references DOSSIER   References nom_gt_binary.pgm (defaut data/images/gt)
  --list FICHIER          Liste des noms d'images, un par ligne, sans extension
  --detector NOM          sobel | laplace | canny | all (all : mode batch)
  --sigma REEL           Ecart-type gaussien [0,5] ; 0 = pas de filtre
  --kernel ENTIER        Taille de derivation : 3, 5 ou 7
  --threshold REEL       Seuil strict sur norme Sobel / valeur absolue Laplace
  --low REEL --high REEL Seuils Canny, 0 <= low <= high (L2gradient=true)
  --tolerance ENTIER     0 = position exacte ; 1 = voisinage 3x3 (defaut)
  --gt-black             Contours noirs sur fond blanc (defaut, base Groningen)
  --gt-white             Contours blancs sur fond noir pour un autre jeu
  --output DOSSIER       Dossier de sortie (defaut results/manual)
  --save-images          Exporter contours et superpositions PNG en mode batch
  --help                 Cette aide
Le CSV contient les mesures exactes ET 3x3 pour chaque image.
Interface : n/p image suivante/precedente, s sauvegarde, b lot, q/Echap quitter.
)";
}
int integer(const std::string& value) {
    std::size_t pos; int result = std::stoi(value, &pos);
    if (pos != value.size()) throw std::invalid_argument("Entier invalide : " + value);
    return result;
}
double real(const std::string& value) {
    std::size_t pos; double result = std::stod(value, &pos);
    if (pos != value.size()) throw std::invalid_argument("Reel invalide : " + value);
    return result;
}
Options parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        std::string key = argv[i];
        auto value = [&]() -> std::string {
            if (++i >= argc) throw std::invalid_argument("Valeur manquante : " + key);
            return argv[i];
        };
        if (key == "--gui") o.gui = true;
        else if (key == "--gui-check") { o.gui = true; o.guiCheck = true; }
        else if (key == "--batch") o.gui = false;
        else if (key == "--save-images") o.saveImages = true;
        else if (key == "--gt-black") o.blackEdges = true;
        else if (key == "--gt-white") o.blackEdges = false;
        else if (key == "--images") o.images = value();
        else if (key == "--references") o.references = value();
        else if (key == "--output") o.output = value();
        else if (key == "--list") o.list = value();
        else if (key == "--detector") o.parameters.detector = value();
        else if (key == "--sigma") o.parameters.sigma = real(value());
        else if (key == "--kernel") o.parameters.kernel = integer(value());
        else if (key == "--threshold") o.parameters.threshold = real(value());
        else if (key == "--low") o.parameters.low = real(value());
        else if (key == "--high") o.parameters.high = real(value());
        else if (key == "--tolerance") o.tolerance = integer(value());
        else throw std::invalid_argument("Option inconnue : " + key);
    }
    auto p = o.parameters;
    if (p.detector == "all") {
        if (o.gui) throw std::invalid_argument("--detector all demande --batch.");
        p.detector = "sobel";
    }
    tp::validate(p);
    if (o.tolerance != 0 && o.tolerance != 1) throw std::invalid_argument("Tolerance : 0 ou 1.");
    return o;
}
std::vector<Sample> loadSamples(const Options& o) {
    std::set<std::string> selection;
    if (!o.list.empty()) {
        std::ifstream file(o.list);
        if (!file) throw std::runtime_error("Liste inaccessible : " + o.list.string());
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty() && line.front() != '#') selection.insert(line);
        }
        if (selection.empty()) throw std::runtime_error("Liste d'images vide.");
    }
    std::vector<fs::path> paths;
    const std::set<std::string> extensions = {".pgm", ".png", ".jpg", ".jpeg", ".bmp", ".tif", ".tiff"};
    for (const auto& entry : fs::directory_iterator(o.images)) {
        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
        if (entry.is_regular_file() && extensions.count(ext)
            && (o.list.empty() || selection.count(entry.path().stem().string()))) paths.push_back(entry.path());
    }
    std::sort(paths.begin(), paths.end());
    std::set<std::string> found;
    std::vector<Sample> samples;
    for (const auto& path : paths) {
        Sample s; s.name = path.stem().string();
        if (!found.insert(s.name).second) throw std::runtime_error("Nom d'image duplique : " + s.name);
        s.image = cv::imread(path.string(), cv::IMREAD_GRAYSCALE);
        fs::path refPath;
        for (const auto& suffix : {"_gt_binary.pgm", "_gt_binary.png", ".png", ".pgm"}) {
            auto candidate = o.references / (s.name + suffix);
            if (fs::is_regular_file(candidate)) { refPath = candidate; break; }
        }
        if (refPath.empty()) throw std::runtime_error("Reference manquante pour " + s.name);
        auto raw = cv::imread(refPath.string(), cv::IMREAD_GRAYSCALE);
        if (s.image.empty() || raw.empty()) throw std::runtime_error("Lecture impossible : " + s.name);
        if (raw.size() != s.image.size()) throw std::runtime_error("Dimensions incompatibles : " + s.name);
        s.reference = tp::binaryReference(raw, o.blackEdges);
        samples.push_back(std::move(s));
    }
    if (samples.empty()) throw std::runtime_error("Aucune paire image/reference.");
    if (!o.list.empty() && found != selection) throw std::runtime_error("Des noms de la liste ne sont pas presents.");
    return samples;
}
std::string csvEscape(const std::string& value) {
    std::string result = "\"";
    for (char c : value) { result += c; if (c == '"') result += '"'; }
    return result + "\"";
}
void savePng(const fs::path& path, const cv::Mat& image) {
    if (!cv::imwrite(path.string(), image)) throw std::runtime_error("Ecriture impossible : " + path.string());
}
void batch(const Options& o, const std::vector<Sample>& samples) {
    fs::create_directories(o.output);
    std::ofstream csv(o.output / "metrics.csv");
    if (!csv) throw std::runtime_error("Impossible d'ecrire metrics.csv.");
    csv << "image,detector,sigma,kernel,threshold,low,high,radius,detected,reference,correct,fp,fn,P,TFP,TFN,detector_ms\n";
    csv << std::setprecision(10);
    std::vector<std::string> detectors = o.parameters.detector == "all"
        ? std::vector<std::string>{"sobel", "laplace", "canny"}
        : std::vector<std::string>{o.parameters.detector};
    for (const auto& detector : detectors) for (const auto& s : samples) {
        auto p = o.parameters; p.detector = detector;
        auto start = std::chrono::steady_clock::now();
        auto edges = tp::detect(s.image, p);
        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        for (int radius : {0, 1}) {
            auto e = tp::evaluate(edges, s.reference, radius);
            const auto& m = e.metrics;
            csv << csvEscape(s.name) << ',' << detector << ',' << p.sigma << ',' << p.kernel << ','
                << p.threshold << ',' << p.low << ',' << p.high << ',' << radius << ','
                << m.detected << ',' << m.reference << ',' << m.correct << ',' << m.fp << ',' << m.fn << ','
                << m.performance << ',' << m.falsePositiveRate << ',' << m.falseNegativeRate << ',' << ms << '\n';
            if (o.saveImages && radius == o.tolerance) {
                savePng(o.output / (s.name + "_" + detector + "_edges.png"), edges);
                savePng(o.output / (s.name + "_" + detector + "_overlay.png"), tp::overlay(e, edges, s.reference));
            }
        }
    }
    csv.close();
    if (!csv) throw std::runtime_error("Erreur d'ecriture du CSV.");
    std::cout << samples.size() << " images x " << detectors.size() << " detecteur(s) : "
              << (o.output / "metrics.csv") << '\n';
}
void label(cv::Mat& panel, const std::string& text, int x, int y, double size = 0.55,
           cv::Scalar color = cv::Scalar(225, 225, 225)) {
    cv::putText(panel, text, cv::Point(x, y), cv::FONT_HERSHEY_SIMPLEX, size, color, 1, cv::LINE_AA);
}
cv::Mat panelFor(const Sample& sample, const cv::Mat& edges, const tp::Evaluation& e,
                 const tp::Parameters& p, int radius) {
    cv::Mat panel(680, 1040, CV_8UC3, cv::Scalar(25, 25, 28));
    label(panel, "TP CONTOURS  /  " + sample.name + "  /  " + p.detector, 22, 30, 0.75);
    std::vector<cv::Mat> images = {sample.image, sample.reference, edges, tp::overlay(e, edges, sample.reference)};
    const std::vector<std::string> titles = {"Image originale", "Contours de reference", "Contours detectes", "Comparaison : vert=correct, rouge=FP, bleu=FN"};
    for (int i = 0; i < 4; ++i) {
        int x = 16 + (i % 2) * 520, y = 67 + (i / 2) * 274;
        cv::Mat tile;
        if (images[i].channels() == 1) cv::cvtColor(images[i], tile, cv::COLOR_GRAY2BGR);
        else tile = images[i];
        double scale = std::min(500.0 / tile.cols, 240.0 / tile.rows);
        cv::Mat resized;
        cv::resize(tile, resized, cv::Size(), scale, scale, cv::INTER_NEAREST);
        resized.copyTo(panel(cv::Rect(x + (500 - resized.cols) / 2, y, resized.cols, resized.rows)));
        label(panel, titles[i], x, y - 8, i == 3 ? 0.44 : 0.5);
    }
    const auto& m = e.metrics;
    std::ostringstream text;
    text << std::fixed << std::setprecision(3) << "P=" << m.performance << "   TFP="
         << m.falsePositiveRate << "   TFN=" << m.falseNegativeRate
         << "   corrects=" << m.correct << "   tolerance=" << radius;
    label(panel, text.str(), 20, 620, 0.55);
    text.str(""); text.clear();
    text << "sigma=" << p.sigma << "  noyau=" << p.kernel << "  seuil=" << p.threshold
         << "  Canny=" << p.low << "/" << p.high;
    label(panel, text.str(), 20, 646, 0.5);
    label(panel, "n/p : image   s : sauvegarder   b : traiter le lot   q/Echap : quitter", 20, 670, 0.48);
    return panel;
}
void gui(Options o, const std::vector<Sample>& samples) {
    const std::string window = "TP_contours - Reglages";
    const std::vector<std::string> detectors = {"sobel", "laplace", "canny"};
    cv::namedWindow(window, cv::WINDOW_AUTOSIZE);
    const std::vector<std::pair<std::string, int>> controls = {
        {"Detecteur 0=S 1=L 2=C", 2}, {"Image", std::max(1, static_cast<int>(samples.size()) - 1)},
        {"Sigma x10", 50}, {"Noyau 0=3 1=5 2=7", 2}, {"Seuil Sobel/Laplace", 500},
        {"Canny bas", 500}, {"Canny haut", 1000}, {"Tolerance 0=exact 1=3x3", 1}};
    for (const auto& control : controls) cv::createTrackbar(control.first, window, nullptr, control.second);
    std::vector<int> initial = {static_cast<int>(std::find(detectors.begin(), detectors.end(), o.parameters.detector) - detectors.begin()),
        0, static_cast<int>(o.parameters.sigma * 10), (o.parameters.kernel - 3) / 2,
        static_cast<int>(std::min(o.parameters.threshold, 500.0)), static_cast<int>(std::min(o.parameters.low, 500.0)),
        static_cast<int>(std::min(o.parameters.high, 1000.0)), o.tolerance};
    for (std::size_t i = 0; i < controls.size(); ++i) cv::setTrackbarPos(controls[i].first, window, initial[i]);
    if (o.guiCheck) {
        fs::create_directories(o.output);
        std::ofstream proof(o.output / "gui_check.txt");
        for (int detector = 0; detector < 3; ++detector) {
            cv::setTrackbarPos(controls[0].first, window, detector);
            cv::setTrackbarPos(controls[2].first, window, 20);
            cv::setTrackbarPos(controls[4].first, window, 40 + 40 * detector);
            cv::waitKey(100);
            if (cv::getTrackbarPos(controls[0].first, window) != detector
                || cv::getTrackbarPos(controls[2].first, window) != 20
                || cv::getTrackbarPos(controls[4].first, window) != 40 + 40 * detector)
                throw std::runtime_error("Echec aller-retour des curseurs.");
            auto p = o.parameters; p.detector = detectors[detector]; p.sigma = 2;
            p.threshold = cv::getTrackbarPos(controls[4].first, window);
            auto edges = tp::detect(samples.front().image, p);
            auto e = tp::evaluate(edges, samples.front().reference, 1);
            auto panel = panelFor(samples.front(), edges, e, p, 1);
            cv::imshow(window, panel); cv::waitKey(100);
            savePng(o.output / (p.detector + "_panel.png"), panel);
            proof << p.detector << ": curseurs lus, traitement, rendu et PNG OK\n";
        }
        proof.close();
        cv::destroyAllWindows();
        std::cout << "Verification graphique terminee : " << o.output << '\n';
        return;
    }
    std::vector<int> previous;
    cv::Mat panel, edges;
    tp::Evaluation evaluation;
    int selected = 0;
    for (;;) {
        std::vector<int> state;
        for (const auto& control : controls) state.push_back(cv::getTrackbarPos(control.first, window));
        if (state != previous) {
            selected = std::min(state[1], static_cast<int>(samples.size()) - 1);
            o.parameters.detector = detectors[state[0]];
            o.parameters.sigma = state[2] / 10.0; o.parameters.kernel = 3 + 2 * state[3];
            o.parameters.threshold = state[4]; o.parameters.low = state[5];
            o.parameters.high = std::max(state[5], state[6]); o.tolerance = state[7];
            if (state[6] < state[5]) cv::setTrackbarPos(controls[6].first, window, state[5]);
            edges = tp::detect(samples[selected].image, o.parameters);
            evaluation = tp::evaluate(edges, samples[selected].reference, o.tolerance);
            panel = panelFor(samples[selected], edges, evaluation, o.parameters, o.tolerance);
            cv::imshow(window, panel); previous = state;
        }
        int key = cv::waitKey(30) & 255;
        if (key == 27 || key == 'q' || cv::getWindowProperty(window, cv::WND_PROP_VISIBLE) < 1) break;
        if (key == 'n' || key == 'p') {
            selected = (selected + (key == 'n' ? 1 : static_cast<int>(samples.size()) - 1)) % samples.size();
            cv::setTrackbarPos(controls[1].first, window, selected);
        }
        if (key == 's') {
            // Sauvegarde aussi les parametres et les deux evaluations dans le CSV.
            auto saved = o;
            saved.output /= samples[selected].name + "_" + o.parameters.detector;
            saved.saveImages = true;
            batch(saved, {samples[selected]});
            savePng(saved.output / "interface.png", panel);
        }
        if (key == 'b') { auto saved = o; saved.saveImages = true; batch(saved, samples); }
    }
    cv::destroyAllWindows();
}
int main(int argc, char** argv) {
    try {
        for (int i = 1; i < argc; ++i) if (std::string(argv[i]) == "--help") { help(); return 0; }
        cv::setNumThreads(1);
        auto options = parse(argc, argv);
        auto samples = loadSamples(options);
        if (options.gui) gui(options, samples); else batch(options, samples);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Erreur : " << error.what() << '\n'; return 1;
    }
}
