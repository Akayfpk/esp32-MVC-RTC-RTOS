# Design Notes

Living document for non-obvious decisions, deferred refactors, and tooling
quirks. Update atomically when the underlying assumption changes.

## Deferred refactor: collapse state dispatch into a table

**Trigger to revisit:** the day a 7th `SystemState` is added, or a 3rd
display surface (e.g. web UI, serial console renderer) appears.

**Why deferred:** today, adding one menu item with a real screen requires
coordinated edits across roughly nine files:

- `src/Model.cpp` — append label to `m_menuItems[]`
- `src/Model.h` — add value to `SystemState` enum
- `src/Controller.h` / `.cpp` — add menu-index case, router case, handler
- `src/View.h` / `.cpp` — add virtual render method + default body
- `src/OledView.h` / `.cpp` — override, draw method, switch case
- `src/LcdView.h` / `.cpp` — override, display method, switch case

For reference, commit `1a64e88` (adding `STATE_INPUTS` and `STATE_OUTPUTS`)
touched 9 files / ~140 lines.

**Proposed replacement:** a single `MenuEntry` table where each row owns
its label and three function pointers (OLED render, LCD render, on-select).
Dispatch becomes `entry->onSelect(model)` and `entry->onOledRender(oled)`
instead of a switch over the enum. Adding a state then costs ~10 lines in
one file.

**Trade-offs disclosed:**

| Axis                                | Switch today      | Table proposed         |
|-------------------------------------|-------------------|------------------------|
| Add a state                         | ~9 files          | 1 file                 |
| Add a new operation across states   | 1 switch          | new column on N rows   |
| Compiler exhaustiveness (`-Wswitch`) | yes              | no (silent null ptrs)  |
| Code size                           | smaller           | ~6 ptrs/row + indirect |
| GDB readability                     | static functions  | indirect through table |

Do not refactor pre-emptively. The switch form is defensible at the current
size (6 states, 2 displays) and the compiler safety it provides is real.

## Wokwi tooling quirks

- **Serial output (Wokwi 3.5.0)** does not forward UART bytes through the
  RFC2217 server on port 4000 nor through the Wokwi Terminal pseudoterminal.
  Do not spend time re-debugging this. Use the LCD HUD or GDB instead.
- **GDB stub** runs on port 3333 (enabled in `wokwi.toml`). FreeRTOS
  OS-aware plugin is not loaded — `info threads` shows only the two CPU
  contexts (`prvIdleTask` / `esp_pm_impl_waiti`), not per-task threads.
  To inspect tasks, call FreeRTOS internals manually from GDB or add
  explicit probes in code.
- **LTO** strips unused symbols. `esp_get_free_heap_size`, `ESP.getFreeHeap`,
  and similar are invisible to GDB unless reachable from a live call site.
  Add a probe in your code, or read the values from a known struct, when
  needed.

## Engineering protocol

Changes follow the Measured Change Protocol (see user prompt
`measured-change.prompt.md`). Every change gates on six fields: Symptom /
Measurement / Hypothesis / Smallest change / Expected post-change
measurement / Revert plan. One change per commit. Revert plan is always
`git revert <sha>` or `git reset --hard <baseline-sha>`.

Baseline tags:

- `93515d1` — pristine origin/master + protocol adoption
- `c8b2298` — tooling-only: Wokwi GDB port enabled
