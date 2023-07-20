from __future__ import annotations

from pathlib import Path

import pandas as pd
import pytest

from liburlparser import Host

host_data_df = pd.read_csv(Path(__file__).parent / "data" / "host_data.csv", keep_default_na=False)


@pytest.mark.parametrize("host_data", map(pd.Series.to_dict, host_data_df.iloc))
def test_host(host_data):
    host = Host.from_url(host_data["url"])
    assert str(host) == host_data["host"]
    assert host.domain == host_data["domain"]
    assert host.domain_name == host_data["domain_name"]
    assert host.suffix == host_data["suffix"]
