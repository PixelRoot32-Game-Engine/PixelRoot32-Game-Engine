# Game Objective **2048**

---

## Main Objective

* **Create a tile with the value 2048** by combining tiles of the same number.

---

## How it's Achieved

* You slide all tiles in one direction.
* When two equal tiles touch, they combine:

  * 2 + 2 → 4
  * 4 + 4 → 8
  * 8 + 8 → 16
  * …
* Repeat the process until you reach **2048**.

---

## Extended Objective (actual in practice)

Although you "win" when you reach 2048:

* The game **does not end mandatory**
* You can continue playing to:

  * get larger tiles (4096, 8192, …)
  * maximize your score

---

## Losing Condition

You lose when:

* the board is full, **and**
* no moves or combinations are possible

---

## In Summary

* Formal objective: **reach 2048**
* Practical objective: **survive as long as possible and maximize value/score**
