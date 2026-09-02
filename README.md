# Parallel Search Engine

A parallel search engine written in C++ with an inverted index, multithreaded document processing, and local LLM integration through Ollama.

## Dataset setup

Dataset files are not included in the Git repository because of their size. 

To prepare the dataset:
1. Place the downloaded archive in: `data/archives/`
2. Extract the archive into: `data/raw/` 
3. Verify that the resulting directory structure looks like this: 
```text
data/
├── archives/
│   └── D184MB.zip
└── raw/
    └── D184MB/
        ├── 2.txt
        ├── 10.txt
        ├── 11.txt
        └── ...
```
Each `.txt`  file is treated as a separate document.

The contents of `data/archives/` and `data/raw` are excluded from Git. The `.gitkeep` files preserve the required directory structure after cloning the repository.