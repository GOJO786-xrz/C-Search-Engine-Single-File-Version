
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <dirent.h>   // POSIX directory listing (no C++17 needed)

static const std::unordered_set<std::string> kStopWords = {
    "a","an","and","are","as","at","be","by","for","from","has","he","in","is",
    "it","its","of","on","that","the","to","was","were","will","with","this",
    "but","or","not","you","your","i","we","they","do","does","did","so","if"
};

class SearchEngine {
public:
    void addDocument(const std::string& docId, const std::string& content) {
        documents_[docId] = content;

        int length = 0;
        std::vector<std::string> tokens = tokenize(content);
        for (std::size_t i = 0; i < tokens.size(); ++i) {
            const std::string& token = tokens[i];
            if (token.size() < 2 || isStopWord(token)) continue;
            index_[token][docId] += 1;
            ++length;
        }
        docLengths_[docId] = length;
    }

    void loadDirectory(const std::string& path) {
        DIR* dir = opendir(path.c_str());
        if (!dir) {
            std::cerr << "Directory not found: " << path << "\n";
            return;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;

            // Only .txt files
            if (name.size() < 4 || name.substr(name.size() - 4) != ".txt")
                continue;

            std::string fullPath = path + "/" + name;
            std::ifstream file(fullPath.c_str());
            if (!file) continue;

            std::stringstream buffer;
            buffer << file.rdbuf();
            addDocument(name, buffer.str());
        }
        closedir(dir);

        std::cout << "Loaded " << documents_.size() << " document(s).\n";
    }

    std::vector<std::pair<std::string, double> >
    search(const std::string& query, std::size_t topK = 10) const {
        std::unordered_map<std::string, int> queryTerms;
        std::vector<std::string> qTokens = tokenize(query);
        for (std::size_t i = 0; i < qTokens.size(); ++i) {
            const std::string& token = qTokens[i];
            if (token.size() < 2 || isStopWord(token)) continue;
            queryTerms[token]++;
        }

        if (queryTerms.empty())
            return std::vector<std::pair<std::string, double> >();

        std::unordered_map<std::string, double> scores;

        for (const auto& kv : queryTerms) {
            const std::string& term = kv.first;
            const int qCount = kv.second;

            std::unordered_map<std::string,
                std::unordered_map<std::string, int> >::const_iterator it =
                index_.find(term);
            if (it == index_.end()) continue;

            const double termIdf = idf(term);
            const double queryWeight =
                (1.0 + std::log(static_cast<double>(qCount))) * termIdf;

            for (const auto& kv2 : it->second) {
                const std::string& docId    = kv2.first;
                const int          termFreq = kv2.second;

                const double docWeight =
                    (1.0 + std::log(static_cast<double>(termFreq))) * termIdf;

                const int len = docLengths_.at(docId);
                const double norm =
                    (len > 0) ? 1.0 / std::sqrt(static_cast<double>(len)) : 1.0;

                scores[docId] += queryWeight * docWeight * norm;
            }
        }

        std::vector<std::pair<std::string, double> > ranked(scores.begin(),
                                                            scores.end());
        std::sort(ranked.begin(), ranked.end(),
                  [](const std::pair<std::string, double>& a,
                     const std::pair<std::string, double>& b) {
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
                       std::unordered_map<std::string, int> > index_;
    std::unordered_map<std::string, int> docLengths_;

    static std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::string current;

        for (std::size_t i = 0; i < text.size(); ++i) {
            const unsigned char uc = static_cast<unsigned char>(text[i]);
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
        std::unordered_map<std::string,
            std::unordered_map<std::string, int> >::const_iterator it =
            index_.find(term);
        if (it == index_.end()) return 0.0;

        const double N  = static_cast<double>(documents_.size());
        const double df = static_cast<double>(it->second.size());
        return std::log((N + 1.0) / (df + 1.0)) + 1.0;
    }
};

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

        std::vector<std::pair<std::string, double> > results =
            engine.search(line, 10);

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
