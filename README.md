#rarexsec_analysis

Framework for rare cross-section measurements.

## Build
```bash
source .container.sh
source .setup.sh
source .build.sh
```

## Configure
```bash
python configurate.py
```
Generates `config/config.json` by processing the sample definitions in `config/samples.json` and hadding inputs as needed.
`config/samples.json` lists input samples, detector variations, and metadata such as file paths and nominal POT.

## Run
```bash
./build/analyse config/config.json [config/plugins/<plugins.json>]
```
Runs the analysis using `config/config.json` and optional plugin definitions from `config/plugins/`.

## Test
```bash
ctest --output-on-failure
```

## Example JSON plugins
```json
{
    "bins" : {
        "mode" : "dynamic",
        "min" : 0.0,
        "max" : 3000.0,
        "include_out_of_range_bins" : true,
        "strategy" : "freedman_diaconis"
    }
}
```

The `strategy` field accepts several common rules for choosing the number of
bins: `equal_weight`, `freedman_diaconis`, `scott`, `sturges`, `rice`, or
`sqrt`.

```json
{
    "path" : "build/EventDisplayPlugin.so", "event_displays" : [ {
        "sample" : "sample",
        "region" : "REGION_KEY",
        "n_events" : 5,
        "image_size" : 800,
        "output_directory" : "./plots"
    } ]
}
```

```json {
    "path" : "build/RocCurvePlugin.so", "roc_curves" : [ {
        "region" : "REGION_KEY",
        "selection_rule" : "NUMU_CC",
        "channel_column" : "incl_channel",
        "signal_group" : "inclusive_strange_channels",
        "variable" : "some_discriminant",
        "output_directory" : "./plots",
        "plot_name" : "roc_curve"
    } ]
}
```
