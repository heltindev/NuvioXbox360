"""Tests for the stdlib-only IMDb metadata importer (no network requests)."""

import csv
import gzip
from pathlib import Path
import tempfile
import unittest

from import_imdb import ascii_title, build_catalog


class ImportImdbTests(unittest.TestCase):
    def test_normalization(self):
        self.assertEqual(ascii_title("Caf\u00e9\tSociety"), "Cafe Society")

    def test_small_gzip_files(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory) / "title.basics.tsv.gz"
            rating = Path(directory) / "title.ratings.tsv.gz"
            with gzip.open(base, "wt", encoding="utf-8") as file:
                writer = csv.writer(file, delimiter="\t")
                writer.writerow(["tconst", "titleType", "primaryTitle", "originalTitle", "isAdult", "startYear", "endYear", "runtimeMinutes", "genres"])
                writer.writerow(["tt0000001", "movie", "Night Voyage", "Night Voyage", "0", "2024", "\\N", "90", "Drama"])
                writer.writerow(["tt0000002", "tvSeries", "Clear Skies", "Clear Skies", "0", "2025", "\\N", "30", "Drama"])
                writer.writerow(["tt0000003", "movie", "Forbidden", "Forbidden", "1", "2024", "\\N", "90", "Horror"])
            with gzip.open(rating, "wt", encoding="utf-8") as file:
                writer = csv.writer(file, delimiter="\t")
                writer.writerow(["tconst", "averageRating", "numVotes"])
                writer.writerow(["tt0000001", "8.2", "100000"])
                writer.writerow(["tt0000002", "9.0", "200000"])
                writer.writerow(["tt0000003", "10.0", "200000"])
            rows = build_catalog(base, rating, 10, 25000)
            self.assertEqual([row[0] for row in rows], ["tt0000001", "tt0000002"])
            self.assertEqual([row[1] for row in rows], ["movie", "series"])


if __name__ == "__main__":
    unittest.main()
