# IMDb titles on NUVIO.XEX (beginner guide)

NUVIO.XEX currently shows a **fictional demo catalog** when it starts. If you want the titles and years of actual films and television shows, you can import a **small offline selection** from IMDb's noncommercial datasets. This does **not** download movies, TV episodes or pictures of posters.

## What you need

- A Windows PC with Python 3 installed (the Xbox itself does not need Python).
- The `NuvioXbox360` repository, including the `tools` folder.
- IMDb's two official data files: `title.basics.tsv.gz` and `title.ratings.tsv.gz`.

IMDb provides the files and their conditions at [IMDb Non-Commercial Datasets](https://developer.imdb.com/non-commercial-datasets/). **Read the terms before downloading or using the dataset.** You may not redistribute the original IMDb datasets in this repository, and the importer is intended only for uses allowed by IMDb's noncommercial conditions.

## Four steps on Windows

1. Download `title.basics.tsv.gz` and `title.ratings.tsv.gz` from IMDb's official dataset site. Leave both files compressed (`.gz`).
2. Put both downloads in the main `NuvioXbox360` folder (the folder containing `README.md`).
3. Open a terminal in that folder and run:

   ```powershell
   py -3 tools/import_imdb.py --basics title.basics.tsv.gz --ratings title.ratings.tsv.gz --output catalog.tsv
   ```

4. Copy `catalog.tsv` next to `NUVIO.XEX.xex` in the same folder for testing in Xenia or on Xbox 360. Then start NUVIO.XEX, choose **Admin**, and the banner should read **FROM IMDB DATASET** instead of **FROM DEMO CATALOG**.

To use a smaller selection, add `--limit 50`. You can set `--min-votes 10000` to include titles with fewer IMDb ratings. Importing the full source files on a PC can take some time, depending on disk and processor speed.

## Why not the entire IMDb database now?

The public IMDb TSV datasets include **millions of titles**, far beyond what the first Xbox 360 UI prototype should keep in memory. This first version deliberately loads **up to 240 entries**. Later versions may support indexing, paging and caching large collections.

The public title files provide **names, years, IDs, types and other basic information**, **not licensed poster art or movie video**. The app currently draws colorful procedural poster placeholders. Real poster imagery will require an appropriate separate source, permission/terms review, texture decoding and Xbox-compatible caching.

### Troubleshooting

| What you see | What to check |
|---|---|
| `FROM DEMO CATALOG` | `catalog.tsv` is not found or no valid rows were loaded. Place it beside the `.xex` and restart. |
| Python cannot find the compressed files | Run the terminal from the main repository folder, or provide full file paths. |
| Too few series | Lower `--min-votes`, then run the importer again. |
| Titles lose accented characters | The current Xbox bitmap font only supports a small ASCII alphabet. The importer transliterates names as a temporary compatibility step. |
| A card does not play anything | Playback is not part of this preview; tapping a card opens metadata-only details. |

The data importer is pure Python, uses only the standard library, and does not contact an IMDb server itself. You are responsible for obtaining the datasets and complying with their usage conditions.
