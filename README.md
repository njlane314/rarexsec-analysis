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

## Region presets

Common selection regions can be added via presets. The framework provides a
number of built‑in options:

- `EMPTY` – no event selection (`NONE` rule)
- `QUALITY` – requires the `quality_event` preselection
- `MUON` – selects events containing a reconstructed muon
- `NUMU_CC` – selects charged-current νµ events
- `QUALITY_NUMU_CC` – combines the quality preselection with the νµ CC rule

They are enabled in the `presets` block of the configuration:

```json
{
  "presets": [
    { "name": "QUALITY" },
    { "name": "NUMU_CC" }
  ]
}
```

Each preset configures the `RegionsPlugin` with an analysis region matching the
given selection rule.

## Snapshot preset

To write out events that satisfy the νµ charged‑current selection, enable the
`NUMU_CC_SNAPSHOT` preset. It wires the `SnapshotPlugin` to dump the selected
events to a ROOT file in the `snapshots` directory:

```json
{
  "presets": [
    { "name": "NUMU_CC_SNAPSHOT" }
  ]
}
```

The output file is named `<beam>_<periods>_NUMU_CC_snapshot.root`.

## PipelineBuilder usage

When constructing a pipeline with presets, the `PipelineBuilder` now requires
at least one variable and one region preset before adding plot presets. Region
and variable presets are added with dedicated calls to make the intended
structure explicit:

```cpp
PipelineBuilder builder(analysis_host, plot_host);
builder.region("TRUE_NEUTRINO_VERTEX");
builder.region("RECO_NEUTRINO_VERTEX");
builder.variable("EMPTY");
builder.preset("NEUTRINO_VERTEX_STACKED_PLOTS");
```

## Run Periods

1. Run 1 → October 2015 to July 2016
2. Run 2 → October 2016 to July 2017
3. Run 3 → October 2017 to July 2018
4. Run 4 → October 2018 to July 2019
5. Run 5 → October 2019 to March 2020

```bash
====================================================================================================
BNB & NuMI POT / Toroid by run period
FAST SQL path (sqlite3); DBROOT: /exp/uboone/data/uboonebeam/beamdb
NuMI FHC/RHC splits from /exp/uboone/app/users/guzowski/slip_stacking/numi_v4.db
====================================================================================================
Run 1  (2015-10-01T00:00:00 → 2016-07-31T23:59:59)

Run     | BNB    |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
1       | TOTAL  |     40062895.0     87344880.0     89334685.0      3.431e+20      3.427e+20     76797778.0      3.336e+20      3.332e+20

Run     | NuMI   |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
1       | TOTAL  |     40062895.0     13944482.0     14158579.0      4.593e+20      4.582e+20     13419751.0      4.578e+20      4.565e+20
1       | FHC    |            0.0            0.0            0.0              0              0     11843834.0      3.934e+20      3.922e+20
1       | RHC    |            0.0            0.0            0.0              0              0      1509648.0      6.273e+19      6.268e+19

====================================================================================================
Run 2  (2016-10-01T00:00:00 → 2017-07-31T23:59:59)

Run     | BNB    |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
2       | TOTAL  |    165820967.0     75426832.0     82583987.0      3.135e+20      3.131e+20     72891077.0      3.061e+20      3.057e+20

Run     | NuMI   |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
2       | TOTAL  |    165820967.0     10757289.0     11855042.0      4.843e+20      4.828e+20     11037240.0      4.842e+20      4.827e+20
2       | FHC    |            0.0            0.0            0.0              0              0      4586875.0      1.771e+20      1.766e+20
2       | RHC    |            0.0            0.0            0.0              0              0      6225528.0      2.982e+20      2.971e+20

====================================================================================================
Run 3  (2017-10-01T00:00:00 → 2018-07-31T23:59:59)

Run     | BNB    |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
3       | TOTAL  |    194770254.0     78716197.0     79304129.0       3.01e+20      3.017e+20     63920595.0      2.665e+20      2.671e+20

Run     | NuMI   |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
3       | TOTAL  |    194770254.0     12101154.0     12324346.0      5.414e+20      5.392e+20     11463474.0      5.414e+20      5.392e+20
3       | FHC    |            0.0            0.0            0.0              0              0            9.0      4.479e+14      4.466e+14
3       | RHC    |            0.0            0.0            0.0              0              0     11438479.0      5.409e+20      5.387e+20

====================================================================================================
Run 4  (2018-10-01T00:00:00 → 2019-07-31T23:59:59)

Run     | BNB    |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
4       | TOTAL  |    288582511.0     84643581.0     84077913.0      3.478e+20      3.478e+20     75374653.0      3.272e+20      3.273e+20

Run     | NuMI   |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
4       | TOTAL  |    288582511.0     11357660.0     11431516.0      5.298e+20      5.253e+20     11011553.0      5.277e+20      5.232e+20
4       | FHC    |            0.0            0.0            0.0              0              0      4233029.0      2.147e+20      2.126e+20
4       | RHC    |            0.0            0.0            0.0              0              0      6771871.0      3.136e+20      3.111e+20

====================================================================================================
Run 5  (2019-10-01T00:00:00 → 2020-03-31T23:59:59)

Run     | BNB    |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
5       | TOTAL  |    138338167.0     46956608.0     45827708.0      1.741e+20      1.743e+20     32767632.0      1.364e+20      1.366e+20

Run     | NuMI   |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
5       | TOTAL  |    138338167.0      5802346.0      5944044.0      2.521e+20      2.494e+20      5721001.0      2.519e+20      2.493e+20
5       | FHC    |            0.0            0.0            0.0              0              0      5642795.0      2.487e+20      2.461e+20
5       | RHC    |            0.0            0.0            0.0              0              0           94.0      4.698e+15      4.645e+15

====================================================================================================
Legend / definitions
- Row labels:
  - BNB rows show TOTAL only (no horn split available in official BNB DBs).
  - NuMI rows show TOTAL plus FHC and RHC when numi_v4.db is available.
- Time window: rows integrate runs where r.begin_time >= START and r.end_time <= END for each run period.
- Columns (BNB uses Gate2; NuMI uses Gate1):
  - EXT         : SUM(r.EXTTrig)              — number of EXT triggers (from runinfo).
  - Gate        : SUM(r.Gate2Trig or Gate1Trig) — beam gate triggers (runinfo).
  - Cnt         : SUM(r.E1DCNT) for BNB, SUM(r.EA9CNT) for NuMI (raw counter, runinfo).
  - TorA        : SUM(r.tor860)*1e12 (BNB) or SUM(r.tor101)*1e12 (NuMI) — toroid integral from runinfo.
  - TorB/Target : SUM(r.tor875)*1e12 (BNB) or SUM(r.tortgt)*1e12 (NuMI) — second toroid / target toroid from runinfo.
  - Cnt_wcut    : SUM(b.E1DCNT) (BNB) or SUM(n.EA9CNT) (NuMI) — after beam-quality cuts (from bnb_v{1,2}.db or numi_v{1,2}.db).
  - TorA_wcut   : SUM(b.tor860)*1e12 (BNB) or SUM(n.tor101)*1e12 (NuMI) — after beam-quality cuts.
  - TorB/Target_wcut : SUM(b.tor875)*1e12 (BNB) or SUM(n.tortgt)*1e12 (NuMI) — after beam-quality cuts.
- Units: toroid sums are multiplied by 1e12 in SQL; values are printed as scientific/float (POT-scale).
- NuMI FHC/RHC rows: only the *_wcut columns are populated; other columns are printed as 0 by design.
  Those come from numi_v4.db split fields (EA9CNT_fhc/rhc, tor101_fhc/rhc, tortgt_fhc/rhc) joined on (run,subrun).
- DB sources:
  - runinfo table: timing, triggers, and uncuts toroids/counters.
  - bnb_v{1,2}.db / numi_v{1,2}.db: “_wcut” (beam-quality–cut) totals.
  - numi_v4.db: NuMI horn-polarity–split “_wcut” totals (FHC/RHC).
```

## Testing
```bash
ctest --output-on-failure
```
