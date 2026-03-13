import pandas as pd

COLUMNS = [
    "temps_fourmis_ms",
    "temps_evap_ms",
    "temps_update_ms",
]

def main():

    df = pd.read_csv("stats_fourmis.csv")

    print("\nColonnes détectées :")
    for c in df.columns:
        print(f" - {c}")

    time_columns = [c for c in COLUMNS if c in df.columns]

    if not time_columns:
        raise ValueError("Colonnes de temps introuvables dans le fichier CSV")

    print("\n===== RESULTATS =====\n")

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
        print(f"Ecart-type     : {std:.6f} ms")
        print(f"Nb echantillons: {len(values)}\n")

    print("===== GLOBAL =====")
    print(f"Temps total des colonnes analysees : {total_global:.6f} ms")

if __name__ == "__main__":
    main()