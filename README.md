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




    prod_strange_resample_fhc_run1_fhc_reco2_reco2
    New_NuMI_Flux_Run_1_FHC_Pandora_Reco2_reco2_reco2