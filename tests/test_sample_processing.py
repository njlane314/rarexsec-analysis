import pytest

np = pytest.importorskip("numpy")
uproot = pytest.importorskip("uproot")

import config.sample_processing as sp


def test_collect_run_subrun_pairs_from_subrun_tree(tmp_path):
    root_path = tmp_path / "sample.root"
    with uproot.recreate(root_path) as f:
        f["nuselection/SubRun"] = {
            "run": np.array([1, 1], dtype=np.int32),
            "subRun": np.array([2, 3], dtype=np.int32),
        }
    pairs = sp._collect_run_subrun_pairs(root_path)
    assert pairs == {(1, 2), (1, 3)}


def test_collect_run_subrun_pairs_from_generic_tree(tmp_path):
    root_path = tmp_path / "sample.root"
    with uproot.recreate(root_path) as f:
        f["nuselection/OtherTree"] = {
            "run": np.array([4, 4], dtype=np.int32),
            "subRun": np.array([5, 6], dtype=np.int32),
        }
    pairs = sp._collect_run_subrun_pairs(root_path)
    assert pairs == {(4, 5), (4, 6)}


def test_run_command_hadd_fallback(tmp_path, monkeypatch):
    f1 = tmp_path / "a.root"
    f2 = tmp_path / "b.root"
    with uproot.recreate(f1) as f:
        f["tree"] = {"run": np.array([1], dtype=np.int32)}
    with uproot.recreate(f2) as f:
        f["tree"] = {"run": np.array([2], dtype=np.int32)}
    out = tmp_path / "out.root"
    monkeypatch.setattr(sp.shutil, "which", lambda cmd: None)
    assert sp.run_command(["hadd", "-f", str(out), str(f1), str(f2)], True)
    with uproot.open(out) as f:
        arr = f["tree"]["run"].array(library="np")
    assert list(arr) == [1, 2]
