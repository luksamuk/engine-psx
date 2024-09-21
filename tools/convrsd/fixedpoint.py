def tofixed(value: float, scale: int) -> int:
    return int(value * (1 << scale))


def tofixed12(value: float) -> int:
    return tofixed(value, 12)
