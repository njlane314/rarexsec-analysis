#!/usr/bin/env python3
"""Example script to visualise systematic uncertainty contributions.

This reproduces the toy setup from ``tests/test_systematics.cpp`` and
creates a stacked bar chart showing the relative contribution of each
systematic (weight, universe, detector variation) to the total
systematic variance in each bin.
"""

import numpy as np
import matplotlib.pyplot as plt


def main(output: str = "systematics_breakdown.png", normalise: bool = True) -> None:
    """Generate a stacked histogram of systematic contributions.

    Parameters
    ----------
    output:
        Path to the output image file.
    normalise:
        If True, normalise each bin to show fractional contribution.
    """
    # Binning used in the toy example
    edges = np.array([0.0, 1.0, 2.0])
    bin_centers = 0.5 * (edges[:-1] + edges[1:])

    # Covariance matrices from weight, universe, and detector variations
    weight_cov = np.diag([0.04, 0.04])
    universe_cov = np.array([[0.5, -0.5], [-0.5, 0.5]])
    detector_cov = np.array([[0.01, -0.01], [-0.01, 0.01]])

    covs = {
        "weight": weight_cov,
        "universe": universe_cov,
        "detector": detector_cov,
    }

    # Extract variances for each systematic
    variances = {name: np.diag(cov) for name, cov in covs.items()}
    total_variance = np.sum(list(variances.values()), axis=0)

    bottoms = np.zeros_like(bin_centers)
    fig, ax = plt.subplots(figsize=(6, 4))

    for name, color in zip(variances.keys(), ["tab:red", "tab:blue", "tab:green"]):
        values = variances[name]
        if normalise:
            values = values / total_variance
        ax.bar(
            bin_centers,
            values,
            bottom=bottoms,
            width=np.diff(edges),
            label=name,
            color=color,
            edgecolor="black",
        )
        bottoms += values

    ax.set_xlabel("x")
    ax.set_ylabel("Fractional Contribution" if normalise else "Variance")
    ax.legend()
    fig.tight_layout()
    fig.savefig(output)
    print(f"Saved {output}")


if __name__ == "__main__":
    main()
