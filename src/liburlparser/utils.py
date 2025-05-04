def flatten_dict(d: dict):
    items = []
    for k, v in d.items():
        if isinstance(v, dict):
            items.append((k, v.pop("str", None)))
            items.extend(flatten_dict(v).items())
        else:
            items.append((k, v))
    return dict(items)

def to_dict(obj, flatten=False):
    out_dict = obj.to_dict()
    if flatten:
        out_dict = flatten_dict(out_dict)
    return out_dict
    