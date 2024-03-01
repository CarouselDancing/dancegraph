from pathlib import Path
import json
import sys
import os

def dict_raise_on_duplicates(ordered_pairs):
    """Reject duplicate keys."""
    # https://stackoverflow.com/a/14902564
    d = {}
    for k, v in ordered_pairs:
        if k in d:
           raise ValueError("duplicate key: %r" % (k,))
        else:
           d[k] = v
    return d

def list_duplicates(seq):
  seen = set()
  seen_add = seen.add
  # adds all elements it doesn't know yet to seen and all other to seen_twice
  seen_twice = set( x for x in seq if x in seen or seen_add(x) )
  # turn the set into a list (as requested)
  return list( seen_twice )

count = "count" in sys.argv

for path in Path(os.path.dirname(__file__) + "/../resources").rglob('*.json'): 
    try:
    
        d = json.load( open(path, 'rt'), object_pairs_hook=dict_raise_on_duplicates)
        if isinstance(d, list) and d and isinstance(d[0],dict) and "name" in d[0].keys():
            dups = list_duplicates([x["name"] for x in d])
            if dups:
                print("Duplicate elements detected in %s: "%path + ", ".join(dups))
        if count:
            num = len(d)
            relpath = os.path.relpath(path,constants.UNITY_STREAMING_ASSETS_FOLDER)
            print(f"{num:4d} in {relpath}")
    except Exception as e:
        print(f"Problem loading {path}: {e}")