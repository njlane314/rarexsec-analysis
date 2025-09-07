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
./build/analyse config/config.json output.root
./build/analyse config/config.json config/plugins/selection_efficiency.json output.root
./build/plot output.root config/config.json
```
`analyse` executes the analysis and optional plug-ins. `plot` renders figures using the produced ROOT file.

## Example plug-ins
The configuration files contain analysis and plotting directives.

```json
{
  "bins": {
    "mode": "dynamic",
    "min": 0.0,
    "max": 3000.0,
    "include_out_of_range_bins": true,
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
  "path": "build/RocCurvePlugin.so",
  "roc_curves": [
    {
      "region": "REGION_KEY",
      "selection_rule": "NUMU_CC",
      "channel_column": "incl_channel",
      "signal_group": "inclusive_strange_channels",
      "variable": "some_discriminant",
      "output_directory": "./plots",
      "plot_name": "roc_curve"
    }
  ]
}
```

## Testing
```bash
ctest --output-on-failure
```