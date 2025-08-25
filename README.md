# rarexsec_analysis

An analysis framework for rare cross-section measurements.

## Build Instructions

1.  **Clone the repository:**
    ```bash
    git clone <your-repo-url>
    cd rarexsec_analysis
    ```

2.  **Create a build directory:**
    It's good practice to build the project in a separate directory to keep the source tree clean.
    ```bash
    mkdir build
    cd build
    ```

3.  **Run CMake and build:**
    ```bash
    cmake ..
    make -jN
    ```

4.  **Setup the environment and run the analysis:**
    After the build is complete, you need to source the setup script and then you can run the executable.
    ```bash
    source ../.setup.sh
    ./plot_analysis ../config.json
    ```

## Dynamic Binning Strategies

Variables can request automatic binning by setting their `bins` configuration to
`{"mode": "dynamic"}`. By default, the bin edges are chosen so that each bin
contains an equal total weight. To preserve the natural shape of the
distribution, a Freedman–Diaconis strategy is also available:

```json
{
  "bins": {
    "mode": "dynamic",
    "min": 0.0,
    "max": 3000.0,
    "include_out_of_range_bins": true,
    "strategy": "freedman_diaconis"
  }
}
```

Omitting `strategy` or setting it to `equal_weight` retains the original
equal-weight quantile binning.

```json
{
  "path": "build/EventDisplayPlugin.so",
  "event_displays": [
    {
      "sample": "sample",
      "region": "REGION_KEY",
      "n_events": 5,
      "pdf_name": "event_displays.pdf",
      "image_size": 800,
      "output_directory": "plots"
    }
  ]
}
```

### ROC Curve Plugin

Generate signal efficiency versus background rejection plots by sweeping a
threshold on a given variable. The plugin requires the column containing the
channel information, a signal group definition, and the variable to threshold
on:

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
      "output_directory": "plots",
      "plot_name": "roc_curve"
    }
  ]
}
```

