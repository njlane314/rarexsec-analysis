# rarexsec-analysis

## Building
```bash
source .container.sh
source .setup.sh
source .build.sh
```

## Muon neutrino selection

The selection is defined in terms of the following variables:

- `nslice == 1`
- optical filter requirement for simulated events:
  `(_opfilter_pe_beam > 0 && _opfilter_pe_veto < 20)`
  (bypassed when `bnbdata == 1` or `extdata == 1`)
- run‑dependent software trigger for Monte Carlo:
  - before run 16880: `software_trigger_pre > 0` (or `software_trigger_pre_ext > 0` for NuMI)
  - after run 16880: `software_trigger_post > 0` (or `software_trigger_post_ext > 0` for NuMI)
- neutrino vertex inside the fiducial volume:
  - `reco_nu_vtx_sce_x` in `[5, 251]` cm
  - `reco_nu_vtx_sce_y` in `[-110, 110]` cm
  - `reco_nu_vtx_sce_z < 675` cm or `reco_nu_vtx_sce_z > 775` cm
- at least 70% of reconstructed hits in the neutrino slice are within the fiducial volume (contained fraction ≥ 0.7)
- at least 50% of reconstructed hits in the neutrino slice are associated with a Pandora PFParticle (associated hits fraction ≥ 0.5)
- `topological_score > 0.06`
- muon candidate requirements (index `i` over `muon_*` vectors):
  - `muon_trk_score_v[i] > 0.8` and `muon_trk_llr_pid_v[i] > 0.2`
  - `muon_trk_length_v[i] > 10` cm and `muon_trk_distance_v[i] < 4` cm
  - start/end inside fiducial volume:
    - `muon_trk_start_x_v[i]`, `muon_trk_end_x_v[i]` in `[5, 251]` cm
    - `muon_trk_start_y_v[i]`, `muon_trk_end_y_v[i]` in `[-110, 110]` cm
  - `muon_trk_start_z_v[i]`, `muon_trk_end_z_v[i]` in `[20, 986]` cm
  - `muon_pfp_generation_v[i] == 2`
  - `pfp_num_plane_hits_U[i] > 0 && pfp_num_plane_hits_V[i] > 0 &&
    pfp_num_plane_hits_Y[i] > 0`
- event-level: `has_muon` and `n_pfps_gen2 > 1`

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
1       | FHC    |     25649266.0     11930925.0     12111855.0      3.948e+20      3.937e+20     11843834.0      3.934e+20      3.922e+20
1       | RHC    |      9287905.0      1552453.0      1555014.0      6.287e+19      6.282e+19      1509648.0      6.273e+19      6.268e+19

====================================================================================================
Run 2  (2016-10-01T00:00:00 → 2017-07-31T23:59:59)

Run     | BNB    |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
2       | TOTAL  |    165820967.0     75426832.0     82583987.0      3.135e+20      3.131e+20     72891077.0      3.061e+20      3.057e+20

Run     | NuMI   |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
2       | TOTAL  |    165820967.0     10757289.0     11855042.0      4.843e+20      4.828e+20     11037240.0      4.842e+20      4.827e+20
2       | FHC    |     49061579.0      4187060.0      4719105.0      1.774e+20      1.769e+20      4586875.0      1.771e+20      1.766e+20
2       | RHC    |     63045505.0      5955576.0      6460720.0      2.984e+20      2.974e+20      6225528.0      2.982e+20      2.971e+20

====================================================================================================
Run 3  (2017-10-01T00:00:00 → 2018-07-31T23:59:59)

Run     | BNB    |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
3       | TOTAL  |    194770254.0     78716197.0     79304129.0       3.01e+20      3.017e+20     63920595.0      2.665e+20      2.671e+20

Run     | NuMI   |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
3       | TOTAL  |    194770254.0     12101154.0     12324346.0      5.414e+20      5.392e+20     11463474.0      5.414e+20      5.392e+20
3       | FHC    |         4231.0          392.0          392.0      1.953e+16      1.947e+16            9.0      4.479e+14      4.466e+14
3       | RHC    |    131416986.0     11703497.0     11767416.0      5.414e+20      5.392e+20     11438479.0      5.409e+20      5.387e+20

====================================================================================================
Run 4  (2018-10-01T00:00:00 → 2019-07-31T23:59:59)

Run     | BNB    |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
4       | TOTAL  |    288582511.0     84643581.0     84077913.0      3.478e+20      3.478e+20     75374653.0      3.272e+20      3.273e+20

Run     | NuMI   |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
4       | TOTAL  |    288582511.0     11357660.0     11431516.0      5.298e+20      5.253e+20     11011553.0      5.277e+20      5.232e+20
4       | FHC    |     64722925.0      4236078.0      4280229.0      2.147e+20      2.126e+20      4233029.0      2.147e+20      2.126e+20
4       | RHC    |    101008375.0      6827760.0      6850594.0      3.142e+20      3.118e+20      6771871.0      3.136e+20      3.111e+20

====================================================================================================
Run 5  (2019-10-01T00:00:00 → 2020-03-31T23:59:59)

Run     | BNB    |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
5       | TOTAL  |    138338167.0     46956608.0     45827708.0      1.741e+20      1.743e+20     32767632.0      1.364e+20      1.366e+20

Run     | NuMI   |            EXT           Gate            Cnt           TorA    TorB/Target       Cnt_wcut      TorA_wcut TorB/Target_wcut
5       | TOTAL  |    138338167.0      5802346.0      5944044.0      2.521e+20      2.494e+20      5721001.0      2.519e+20      2.493e+20
5       | FHC    |     92602542.0      5610400.0      5739835.0      2.502e+20      2.476e+20      5642795.0      2.487e+20      2.461e+20
5       | RHC    |        30926.0         1803.0         1804.0       9.04e+16      8.938e+16           94.0      4.698e+15      4.645e+15

====================================================================================================
```
