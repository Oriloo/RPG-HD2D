# Système de Combat Tour par Tour — Équilibrage « Triangle de Types »

## 1. Vue d’ensemble

Ce système de combat RPG au tour par tour repose sur **trois types élémentaires** — *Physical (Phys)*, *Elementary (Elem)* et *Spiritual (Spir)* — qui s’opposent dans un triangle d’avantages semblable à « Pierre‑Papier‑Ciseaux ».

### 1.1 Tableau d’efficacité

|                  | **Cible Phys** | **Cible Elem** | **Cible Spir** |
| ---------------- | -------------- | -------------- | -------------- |
| **Attaque Phys** | ×1.0           | ×0.5           | ×2.0           |
| **Attaque Elem** | ×2.0           | ×1.0           | ×0.5           |
| **Attaque Spir** | ×0.5           | ×2.0           | ×1.0           |

> **Avantage** : ×2 (200 % des dégâts de base)
> **Désavantage** : ×0,5 (50 % des dégâts de base)

---

## 2. Statistiques d’une entité

Chaque combattant est décrit par la structure suivante :

```json
{
  "name": "...",     // nom du combattant
  "hpMax": 100,      // points de vie maximum
  "hp": 100,         // points de vie actuels
  "speed": 10,       // détermine l'ordre d'initiative
  "apMax": 4,        // réserve maximale de PA
  "ap": 2,           // points d’action disponibles (gagne +1/tour)
  "buffType": "",    // type de buff en cours (facultatif, atk | def | med)
  "buffDuration": 0, // tours restants du buff
  "type": "phys",    // phys | elem | spir
  "statistics": {
    "physAtk": 100,  // attaque physique
    "physDef": 100,  // défense physique
    "elemAtk": 10,   // attaque élémentaire
    "elemDef": 10,   // défense élémentaire
    "spirAtk": 0,    // attaque spirituelle
    "spirDef": 0     // défense spirituelle
  }
}
```

---

## 3. Structure d’un tour

1. **Phase d’initiative** : trier les combattants par *speed* décroissante ; en cas d’égalité, conserver l’ordre d’inscription (tri stable).
2. **Phase d’action** (par combattant)

    1. Choisir : **Agir** ou **Passer**.
       • *Agir* : sélectionner l’une des 2 – 4 actions propres au combattant (voir § 4).
       • *Passer* : aucune dépense d’AP ce tour.
    2. Résoudre l’action sélectionnée : dégâts ou application du buff. Les effets avec **duration > 1** se répètent/persistent automatiquement sans coût supplémentaire.
    3. Mettre à jour la durée des effets d’état.
3. **Phase de fin de tour** : chaque combattant gagne +1 AP (jusqu’à *apMax*), puis vérifier les conditions de victoire.

---

## 4. Définition des actions

Il **n’existe pas de liste universelle** : chaque combattant possède son propre ensemble d’actions (généralement 2 à 4). Une action est définie par l’objet JSON suivant :

```json
{
  "name": "...",            // nom affiché dans l’UI
  "action": "atk" | "buff", // nature de l’action
  "cost": 1,                // PA nécessaires (1 à ...)
  "type": "phys" | "elem" | "spir" // si action == "atk"
        | "atk"  | "def"  | "med", // si action == "buff"
  "multiplier": 2.4,        // facteur appliqué
  "duration": 1             // nombre de tours où l’effet persiste / se répète
}
```

Le joueur peut enchaîner plusieurs actions tant qu’il dispose des PA nécessaires.

### 4.1 Règles pour les actions **atk**

* **type** doit être *phys*, *elem* ou *spir*.
* **Dégâts** (voir § 5)
* Les dégâts se répètent à chaque tour pendant *duration* tours, sans dépenser d’AP supplémentaires.

### 4.2 Règles pour les actions **buff**

| type    | Effet appliqué                    | Formule                              | Exemple                                                  |
| ------- | --------------------------------- | ------------------------------------ | -------------------------------------------------------- |
| **atk** | Augmente tous les dégâts infligés | atk \* multiplier                    | Buff d’attaque +30 % (multiplier = 0.3) pendant 2 tours  |
| **def** | Réduit les dégâts subis           | def \* *multiplier*                  | Bouclier +50 % (multiplier = 0.5) pendant 1 tour         |
| **med** | Régénère des PV                   | +(*multiplier* × Atkmax) PV par tour | Méditation 20 PV/tour (multiplier = 0.2 si Atkmax = 100) |

* *Atkmax* est la plus grande statistique d’attaque du combattant.
* Les buffs ne se cumulent pas ; appliquer le plus récent.
* Une action de type **buff** met automatiquement fin au tour du combattant.

---

## 5. Calcul des dégâts

$\text{Dégâts} = (\text{Atk}_{type} \times \text{multiplier} \times \text{efficiency}) - \text{Def}_{type}$

* **Atk\*\*\*\*type** : statistique d’attaque correspondant au type de l’action (*physAtk*, *elemAtk*, *spirAtk*).
* **Def\*\*\*\*type** : statistique de défense de la cible correspondant au type de l’action.
* **efficiency** : multiplicateur d’efficacité selon le triangle d’avantage (×2, ×1 ou ×0,5) 
* **multiplier** : multiplicateur issu du tableau d’efficacité.
* **Minimum** : 1 point de dégât.

---

## 6. Ressources & Buffs

* **PV (Points de Vie)** : à 0 → K.O.
* **AP (Action Points)** : stockés jusqu’à *apMax*, +1 par tour.
* **Buffs actifs** : stockés dans *buffType* / *buffDuration* et décrits section 4. Les buffs expirent en fin de tour quand *buffDuration* atteint 0.

---

**Version 0.3** — 22 juin 2025
