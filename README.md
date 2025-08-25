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
5.  **Run test**
    ```bash
    cd build
    ctest --output-on-failure
    ```

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

