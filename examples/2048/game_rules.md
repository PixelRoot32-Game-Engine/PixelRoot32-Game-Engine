# Game Rules **2048**

---

## 1. Board

* **4×4** cell grid.
* Each cell contains:

  * empty, or
  * a power of 2 number (2, 4, 8, 16, …).

---

## 2. Movement Input

* Player can move in 4 directions:

  * up
  * down
  * left
  * right

---

## 3. Basic Movement Rule

When a move is executed:

1. **All tiles slide as far as possible** in the indicated direction.
2. Sliding occurs until:

   * the board edge, or
   * another tile.

---

## 4. Merge Rule

During movement:

* If two **adjacent** tiles in the movement direction have the **same value**, they merge.

Result:

* **One single tile** is created with value:

  `new_value = value * 2`

* The resulting tile stays in the position closest to the edge towards which you're moving.

---

## 5. Key Constraint (very important)

* **A tile can only merge ONCE per move.**

Example:

```
[2, 2, 2, 0] → move left → [4, 2, 0, 0]
NOT → [8, 0, 0, 0]
```

---

## 6. Correct Processing Order

For each row/column (depending on direction):

1. **Slide (compact)** → remove gaps
2. **Merge** → left to right (or according to direction)
3. **Slide again**

---

## 7. New Tile Generation

After a valid move:

* **One new tile** is generated in an empty cell:

  * 90% → value **2**
  * 10% → value **4**

---

## 8. Invalid Move

* If a move:

  * does not slide tiles, **and**
  * produces no merges

→ **Nothing happens**
→ **No new tile is generated**

---

## 9. Losing Condition

Game ends when:

* No empty cells, **and**
* No possible merges in any direction

---

## 10. Winning Condition

* Achieved when a **2048** tile appears
* Game can optionally continue

---

## 11. Key Examples

### Simple Case

```
[2, 0, 2, 0] → left → [4, 0, 0, 0]
```

### Chain Case

```
[2, 2, 4, 4] → left → [4, 8, 0, 0]
```

### Critical Case (avoid double merge)

```
[4, 4, 4, 4] → left → [8, 8, 0, 0]
NOT → [16, 0, 0, 0]
```

---

## 12. Operational Summary (pseudo-flow)

For each input:

```
if move changes state:
    slide()
    merge()
    slide()
    spawn_tile()
```
