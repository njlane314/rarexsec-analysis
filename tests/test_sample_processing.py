import pytest

np = pytest.importorskip("numpy")
uproot = pytest.importorskip("uproot")

from config.sample_processing import _collect_run_subrun_pairs


def test_collect_run_subrun_pairs_from_subrun_tree(tmp_path):
    root_path = tmp_path / "sample.root"
    with uproot.recreate(root_path) as f:
        f["nuselection/SubRun"] = {
            "run": np.array([1, 1], dtype=np.int32),
            "subRun": np.array([2, 3], dtype=np.int32),
        }
    pairs = _collect_run_subrun_pairs(root_path)
    assert pairs == {(1, 2), (1, 3)}


def test_collect_run_subrun_pairs_from_generic_tree(tmp_path):
    root_path = tmp_path / "sample.root"
    with uproot.recreate(root_path) as f:
        f["nuselection/OtherTree"] = {
            "run": np.array([4, 4], dtype=np.int32),
            "subRun": np.array([5, 6], dtype=np.int32),
        }
    pairs = _collect_run_subrun_pairs(root_path)
    assert pairs == {(4, 5), (4, 6)}
