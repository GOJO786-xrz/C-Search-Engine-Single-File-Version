# Simple C++ Search Engine

A lightweight, single-file search engine written in modern C++17. It indexes
`.txt` files from a directory, builds an inverted index, and ranks search
results using **TF-IDF** with length normalization.

## Features

- Single-file implementation (`main.cpp`)
- Inverted index for fast lookup
- TF-IDF ranking with document-length normalization
- English stop-word filtering
- Interactive command-line interface
- No external dependencies (uses only the C++ standard library)

## Build

```bash
g++ -std=c++17 -O2 -o search_engine main.cpp
```

Or with the provided Makefile:

```bash
make
```

## Run

```bash
./search_engine docs
```

Then type a query at the prompt:

```
> modern c++ programming
Top matches:
  1. doc5.txt  [score: 1.83254]
  2. doc1.txt  [score: 1.73012]
```

Type `quit` or `exit` to leave.

## How It Works

1. **Tokenization** — text is split into lowercase alphanumeric tokens.
2. **Stop-word removal** — common English words are discarded.
3. **Indexing** — an inverted index maps each term to `(docId → frequency)`.
4. **Scoring** — TF-IDF:
   - `tf  = 1 + log(term_count)`
   - `idf = log((N + 1) / (df + 1)) + 1`
   - final score is normalized by `1 / sqrt(doc_length)`
5. **Ranking** — documents are sorted by descending score.

## Project Structure

```
.
├── main.cpp
├── docs/            # sample corpus
├── Makefile
└── README.md
```

## Future Improvements

- Boolean operators (`AND`, `OR`, `NOT`)
- Phrase search using token positions
- Stemming (Porter stemmer)
- BM25 ranking
- Persistent on-disk index
- Multithreaded indexing

## License

MIT
