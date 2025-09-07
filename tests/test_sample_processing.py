import pytest

np = pytest.importorskip("numpy")
uproot = pytest.importorskip("uproot")

from config.sample_processing import _collect_run_subrun_pairs


def test_collect_run_subrun_pairs_from_event_tree(tmp_path):
    root_path = tmp_path / "sample.root"
    with uproot.recreate(root_path) as f:
        f["nuselection/BlipRecoAlg"] = {
            "run": np.array([1, 1], dtype=np.int32),
            "subrun": np.array([2, 3], dtype=np.int32),
        }
    pairs = _collect_run_subrun_pairs(root_path)
    assert pairs == {(1, 2), (1, 3)}
