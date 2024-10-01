#!/bin/env python

import sys
from parsers import parse_map
from os.path import realpath
from pprint import pp


def main():
    map_src = realpath(sys.argv[1])
    obj_defs, obj_places = parse_map(map_src)
    for key, d in obj_defs.items():
        d.write()
    obj_places.write()


if __name__ == "__main__":
    main()
