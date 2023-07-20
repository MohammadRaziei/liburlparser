import pytest
from liburlparser import Url
import os
import pandas as pd

cwd = os.path.dirname(os.path.abspath(__file__))

url_data_df = pd.read_csv(os.path.join(cwd, "data", "url_data.csv"), keep_default_na=False)


@pytest.mark.parametrize("url_data", map(pd.Series.to_dict, url_data_df.iloc))
def test_url(url_data):
    url = Url(url_data["url"])
    assert url.protocol == url_data["protocol"]
    assert url.userinfo == url_data["userinfo"]
    assert str(url.host) == url_data["host"]
    assert url.domain == url_data["domain"]
    assert url.domain_name == url_data["domain_name"]
    assert url.suffix == url_data["suffix"]
    assert url.port == url_data["port"]
    assert url.query == url_data["query"]
    assert url.fragment == url_data["fragment"]