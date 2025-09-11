# Configuration Layout

This directory contains configuration assets and helper scripts.

- `data/` – static JSON/YAML files describing datasets and run
  configuration.  These files are consumed by the helper scripts and C++ study programs.
- `scripts/` – Python modules that operate on the configuration data.  For
  example, `build_sample_catalog.py` reads definitions from
  `data/data.json` and writes out `data/samples.json` using utilities from
  `sample_processing.py`.

Scripts reference files via paths relative to this directory, so moving a
configuration file into `data/` automatically makes it available to the
modules in `scripts/`.
