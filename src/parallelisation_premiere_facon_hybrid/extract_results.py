from pathlib import Path
import pandas as pd
import numpy as np

# Colonnes de temps prioritaires si elles existent
PREFERRED_TIME_COLUMNS = [
    "temps_fourmis_ms",
    "temps_evap_ms",
    "temps_update_ms",
]

def load_table():
    """
    Cherche automatiquement un fichier de résultats dans le dossier courant.
    Priorité:
      1) stats_fourmis.csv
      2) stats_fourmis.xlsx
      3) n'importe quel .csv
      4) n'importe quel .xlsx
    """
    candidates = [
        Path("stats_fourmis.csv"),
        Path("stats_fourmis.xlsx"),
    ]

    for p in candidates:
        if p.exists():
            return read_file(p)

    csv_files = sorted(Path(".").glob("*.csv"))
    if csv_files:
        return read_file(csv_files[0])

    xlsx_files = sorted(Path(".").glob("*.xlsx"))
    if xlsx_files:
        return read_file(xlsx_files[0])

    raise FileNotFoundError(
        "Aucun fichier .csv ou .xlsx trouvé dans le dossier courant."
    )

def read_file(path: Path) -> pd.DataFrame:
    print(f"Lecture du fichier : {path}")
    suffix = path.suffix.lower()

    if suffix == ".csv":
        return pd.read_csv(path)

    if suffix == ".xlsx":
        return pd.read_excel(path, engine="openpyxl")

    raise ValueError(f"Format non supporté : {path}")

def find_numeric_time_columns(df: pd.DataFrame):
    """
    Retourne les colonnes à analyser :
    - d'abord les colonnes préférées si elles existent
    - sinon toutes les colonnes numériques sauf 'iteration'
    """
    preferred = [c for c in PREFERRED_TIME_COLUMNS if c in df.columns]
    if preferred:
        return preferred

    numeric_cols = df.select_dtypes(include=[np.number]).columns.tolist()
    numeric_cols = [c for c in numeric_cols if c.lower() not in ("iteration", "it")]
    return numeric_cols

def main():
    df = load_table()

    print("\nColonnes détectées :")
    for c in df.columns:
        print(f" - {c}")

    time_columns = find_numeric_time_columns(df)

    if not time_columns:
        raise ValueError(
            "Aucune colonne numérique exploitable trouvée pour calculer les statistiques."
        )

    print("\n===== RÉSULTATS =====\n")

    total_global = 0.0

    for col in time_columns:
        values = pd.to_numeric(df[col], errors="coerce").dropna()

        total = values.sum()
        mean = values.mean()
        std = values.std(ddof=1) if len(values) > 1 else 0.0

        total_global += total

        print(f"--- {col} ---")
        print(f"Temps total    : {total:.6f} ms")
        print(f"Temps moyen    : {mean:.6f} ms")
        print(f"Écart-type     : {std:.6f} ms")
        print(f"Nb échantillons: {len(values)}\n")

    print("===== GLOBAL =====")
    print(f"Temps total des colonnes analysées : {total_global:.6f} ms")

if __name__ == "__main__":
    main()