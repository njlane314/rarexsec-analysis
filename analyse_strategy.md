# Analysis Strategy and Development Roadmap

## Objective
Establish a robust framework for measuring rare cross sections by isolating
strangeness-containing final states in a muon-neutrino beam. The analysis must
progress from reliable event selection to model-assisted identification while
preserving a transparent treatment of systematic uncertainties.

## Current Capabilities
The existing code base provides:
- a plugin-driven processing chain configured through JSON files;
- quality and $ν_{\mu}$ charged-current selections registered in
  `SelectionRegistry`;
- tools for histogramming, image handling, and systematic aggregation.
These components form the backbone for extending the analysis to rare channels.

## Development Milestones

### 1. Preselection and Charged-Current Refinement
- Tune fiducial and kinematic clauses for the quality and $ν_{\mu}$ CC rules.
- Expose the refined rules to `numu_empty_plugins.json`,
  `numu_quality_pre_plugins.json`, and `numu_plugins.json` for region
  configuration.

### 2. Snapshot for External Machine Learning
- After the initial selection, record a reduced data set containing event
  weights, truth-channel identifiers, detector images, and semantic masks.
- Define a reproducible naming scheme so external training scripts can locate
  the snapshots.

### 3. Binary Classifier Integration
- Introduce processors that load pre-trained binary classifiers and attach score
  branches to each event.
- Employ a one-versus-all approach to tag backgrounds while keeping strange
  channels unseen during training.

### 4. Semantic Segmentation Output
- Add processors that run segmentation models to label pixels associated with
  strange particles.
- Persist the segmentation masks alongside detector images for further
  inspection or downstream cuts.

### 5. Systematic Propagation
- Extend systematic definitions to accommodate new weights and model outputs.
- Ensure covariance matrices incorporate variations from both detector and
  modelling sources.

### 6. Documentation and Workflow Integration
- Document the snapshot schema and the interface for external training and
  inference modules.
- Describe how inference results are reincorporated into the analysis chain.

## Outlook
Executing these milestones will produce an analysis pipeline that progresses
from standard $ν_{\mu}$ CC selection to sophisticated machine-learning-based
identification of strangeness, all while maintaining traceable systematic
uncertainties.
