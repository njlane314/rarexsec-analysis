# rarexsec_analysis

## Building
```bash
source .container.sh
source .setup.sh
source .build.sh
```

## Configuration
```bash
python scripts/generate_analysis_config.py
```
`config/config.json` is assembled from the sample definitions in `config/sample_definitions.json`.

## Running
```bash
./build/analyse config/config.json config/plugins/selection_efficiency.json
# The ROOT file is written to /pnfs/uboone/scratch/users/$USER/analysis_config_<YYYYMMDD>.root
./build/plot /pnfs/uboone/scratch/users/$USER/analysis_config_<YYYYMMDD>.root config/config.json
```
`analyse` executes the analysis and optional plug-ins. By default the output file is named `analysis_<dataset>_<YYYYMMDD>.root`, where `<dataset>` is derived from the samples configuration filename. `plot` renders figures using the produced ROOT file.

## Example plug-ins
The configuration files contain analysis and plotting directives.

```json
{
  "bins": {
    "mode": "dynamic",
    "min": 0.0,
    "max": 3000.0,
    "include_oob_bins": true,
    "strategy": "bayesian_blocks"
  }
}
```

The `strategy` field accepts `equal_weight`, `uniform_width`, or `bayesian_blocks`.

```json
{
  "path": "build/EventDisplayPlugin.so",
  "event_displays": [
    {
      "sample": "sample",
      "region": "REGION_KEY",
      "n_events": 5,
      "image_size": 800,
      "output_directory": "./plots"
    }
  ]
}
```

```json
{
  "path": "build/PerformancePlotPlugin.so",
  "performance_plots": [
    {
      "region": "REGION_KEY",
      "selection_rule": "NUMU_CC",
      "channel_column": "incl_channel",
      "signal_group": "inclusive_strange_channels",
      "variable": "some_discriminant",
      "output_directory": "./plots",
      "plot_name": "performance_plot"
    }
  ]
}
```

## Run Periods

1. Run 1 → October 2015 to July 2016
2. Run 2 → October 2016 to July 2017
3. Run 3 → October 2017 to July 2018
4. Run 4 → October 2018 to July 2019
5. Run 5 → October 2019 to March 2020

## Testing
```bash
ctest --output-on-failure
```