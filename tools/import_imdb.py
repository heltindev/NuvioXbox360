#!/usr/bin/env python3
"""Make a small offline Xbox 360 catalog from officially downloaded IMDb TSVs.

This is not an IMDb API, a poster downloader or a source of video streams.
Read https://developer.imdb.com/non-commercial-datasets/ before using the data.
Only use this importer with data you are entitled to use (personal/noncommercial).
"""

import argparse
import csv
import gzip
import heapq
import math
from pathlib import Path
import unicodedata

XBOX_LIMIT = 240


def open_text(path):
    if str(path).endswith(".gz"):
        return gzip.open(path, "rt", encoding="utf-8", newline="")
    return open(path, "r", encoding="utf-8", newline="")


def read_ratings(path, minimum_votes):
    ratings = {}
    with open_text(path) as source:
        for row in csv.DictReader(source, delimiter="\t"):
            try:
                votes = int(row["numVotes"])
                rating = float(row["averageRating"])
            except (KeyError, ValueError):
                continue
            if votes >= minimum_votes:
                ratings[row["tconst"]] = (rating, votes)
    return ratings


def ascii_title(title):
    title = unicodedata.normalize("NFKD", title)
    title = title.encode("ascii", "ignore").decode("ascii")
    return " ".join(title.replace("\t", " ").replace("\n", " ").split())[:71]


def build_catalog(basics_path, ratings_path, limit, minimum_votes):
    if not 1 <= limit <= XBOX_LIMIT:
        raise ValueError("--limit must be between 1 and 240 (current Xbox limit)")
    ratings = read_ratings(ratings_path, minimum_votes) if ratings_path else {}
    # Reserve space for both categories so neither featured row is empty just
    # because one content type has more ratings entries than the other.
    top_movies = []
    top_series = []
    caps = {"movie": (limit + 1) // 2, "series": limit // 2}
    with open_text(basics_path) as source:
        for row in csv.DictReader(source, delimiter="\t"):
            kind = row.get("titleType")
            if kind not in ("movie", "tvMovie", "tvSeries", "tvMiniSeries"):
                continue
            if row.get("isAdult") != "0":
                continue
            try:
                year = int(row["startYear"])
            except (KeyError, ValueError):
                continue
            if not 1888 <= year <= 2100:
                continue
            title = ascii_title(row.get("primaryTitle", ""))
            if not title:
                continue
            item_type = "movie" if kind in ("movie", "tvMovie") else "series"
            imdb_id = row.get("tconst", "")
            if not imdb_id.startswith("tt"):
                continue
            if ratings_path:
                if imdb_id not in ratings:
                    continue
                rating, votes = ratings[imdb_id]
                score = (rating, math.log1p(votes))
            else:
                # Without ratings the oldest items appear first; pass ratings
                # to get an actual ranking instead.
                score = (year, 0.0)
            value = (score, imdb_id, item_type, year, title)
            bucket = top_movies if item_type == "movie" else top_series
            if not caps[item_type]:
                continue
            if len(bucket) < caps[item_type]:
                heapq.heappush(bucket, value)
            elif value > bucket[0]:
                heapq.heapreplace(bucket, value)
    # Explicit ordering: movie row first, then series row, then the remaining
    # catalog. NUVIO.XEX will group the display by type independently.
    result = sorted(top_movies, reverse=True) + sorted(top_series, reverse=True)
    return [(id_, type_, year, title) for _, id_, type_, year, title in result]


def main():
    parser = argparse.ArgumentParser(description="Create Xbox 360 catalog.tsv from IMDb datasets")
    parser.add_argument("--basics", required=True, type=Path, help="title.basics.tsv.gz")
    parser.add_argument("--ratings", type=Path, help="title.ratings.tsv.gz (recommended)")
    parser.add_argument("--output", type=Path, default=Path("catalog.tsv"))
    parser.add_argument("--limit", type=int, default=XBOX_LIMIT)
    parser.add_argument("--min-votes", type=int, default=25000)
    args = parser.parse_args()
    rows = build_catalog(args.basics, args.ratings, args.limit, args.min_votes)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8", newline="\n") as target:
        for imdb_id, kind, year, title in rows:
            target.write("{}\t{}\t{}\t{}\n".format(imdb_id, kind, year, title))
    print("Imported {} metadata entries into {}".format(len(rows), args.output))
    if len(rows) < 10:
        print("Note: very small catalogs may have empty featured rows.")
    print("Copy catalog.tsv beside the .xex. No videos or posters are included.")


if __name__ == "__main__":
    main()
