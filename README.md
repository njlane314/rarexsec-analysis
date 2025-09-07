# rarexsec_analysis

## Run Periods

1. Run 1 ⇒ 2015-10 to 2016-07
2. Run 2 ⇒ 2016-10 to 2017-07
3. Run 3 ⇒ 2017-10 to 2018-07
4. Run 4 ⇒ 2018-10 to 2019-07
5. Run 5 ⇒ 2019-10 to 2020-03

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

## Testing
```bash
ctest --output-on-failure
```