/*
 * Simple C++ Search Engine
 * ------------------------
 * A lightweight, in-memory search engine that:
 *   - Loads .txt documents from a directory
 *   - Builds an inverted index
 *   - Ranks results using TF-IDF with length normalization
 *   - Supports interactive queries from the terminal
 *
 * Build:  g++ -std=c++17 -O2 -o search_engine main.cpp
 * Run:    ./search_engine docs
 *
 * Author: <your-name>
 * License: MIT
 */

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Common English stop words (ignored during indexing & querying)
// ---------------------------------------------------------------------------
static const std::unordered_set<std::string> kStopWords = {
    "a","an","and","are","as","at","be","by","for","from","has","he","in","is",
    "it","its","of","on","that","the","to","was","were","will","with","this",
    "but","or","not","you","your","i","we","they","do","does","did","so","if"
};

// ---------------------------------------------------------------------------
// SearchEngine
// ---------------------------------------------------------------------------
class SearchEngine {
public:
    // Add a single document to the index
    void addDocument(const std::string& docId, const std::string& content) {
        documents_[docId] = content;

        int length = 0;
        for (const auto& token : tokenize(content)) {
            if (token.size() < 2 || isStopWord(token)) continue;
            index_[token][docId] += 1;
            ++length;
        }
        docLengths_[docId] = length;
    }

    // Load every .txt file inside a directory
    void loadDirectory(const std::string& path) {
        if (!fs::exists(path) || !fs::is_directory(path)) {
            std::cerr << "Directory not found: " << path << "\n";
            return;
        }

        for (const auto& entry : fs::directory_iterator(path)) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".txt") continue;

            std::ifstream file(entry.path());
            if (!file) continue;

            std::stringstream buffer;
            buffer << file.rdbuf();
            addDocument(entry.path().filename().string(), buffer.str());
        }

        std::cout << "Loaded " << documents_.size() << " document(s).\n";
    }

    // Search: returns (docId, score) sorted by descending relevance
    std::vector<std::pair<std::string, double>>
    search(const std::string& query, std::size_t topK = 10) const {
        // Build query term frequencies
        std::unordered_map<std::string, int> queryTerms;
        for (const auto& token : tokenize(query)) {
            if (token.size() < 2 || isStopWord(token)) continue;
            queryTerms[token]++;
        }

        if (queryTerms.empty()) return {};

        std::unordered_map<std::string, double> scores;

        for (const auto& [term, qCount] : queryTerms) {
            auto it = index_.find(term);
            if (it == index_.end()) continue;

            const double termIdf = idf(term);
            const double queryWeight =
                (1.0 + std::log(static_cast<double>(qCount))) * termIdf;

            for (const auto& [docId, termFreq] : it->second) {
                const double docWeight =
                    (1.0 + std::log(static_cast<double>(termFreq))) * termIdf;

                const int len = docLengths_.at(docId);
                const double norm =
                    (len > 0) ? 1.0 / std::sqrt(static_cast<double>(len)) : 1.0;

                scores[docId] += queryWeight * docWeight * norm;
            }
        }

        std::vector<std::pair<std::string, double>> ranked(scores.begin(),
                                                           scores.end());
        std::sort(ranked.begin(), ranked.end(),
                  [](const auto& a, const auto& b) {
                      return a.second > b.second;
                  });

        if (ranked.size() > topK) ranked.resize(topK);
        return ranked;
    }

    void printStats() const {
        std::cout << "Documents : " << documents_.size() << "\n"
                  << "Terms     : " << index_.size() << "\n";
    }

private:
    std::unordered_map<std::string, std::string> documents_;
    std::unordered_map<std::string,
                       std::unordered_map<std::string, int>> index_;
    std::unordered_map<std::string, int> docLengths_;

    // --- helpers -----------------------------------------------------------
    static std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::string current;

        for (char c : text) {
            const auto uc = static_cast<unsigned char>(c);
            if (std::isalnum(uc)) {
                current += static_cast<char>(std::tolower(uc));
            } else if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        }
        if (!current.empty()) tokens.push_back(current);
        return tokens;
    }

    static bool isStopWord(const std::string& word) {
        return kStopWords.count(word) > 0;
    }

    double idf(const std::string& term) const {
        auto it = index_.find(term);
        if (it == index_.end()) return 0.0;

        const double N  = static_cast<double>(documents_.size());
        const double df = static_cast<double>(it->second.size());
        return std::log((N + 1.0) / (df + 1.0)) + 1.0;   // smoothed IDF
    }
};

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    const std::string docsPath = (argc > 1) ? argv[1] : "docs";

    SearchEngine engine;
    engine.loadDirectory(docsPath);
    engine.printStats();

    std::cout << "\nType a query (or 'quit' to exit):\n";
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "quit" || line == "exit") break;
        if (line.empty()) continue;

        const auto results = engine.search(line, 10);
        if (results.empty()) {
            std::cout << "No results found.\n\n";
            continue;
        }

        std::cout << "\nTop matches:\n";
        for (std::size_t i = 0; i < results.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << results[i].first
                      << "  [score: " << results[i].second << "]\n";
        }
        std::cout << "\n";
    }

    return 0;
}
