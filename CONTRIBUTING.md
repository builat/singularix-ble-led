# Contributing

Thanks for contributing!

## Guidelines
- Keep the main loop non-blocking (no `delay()`, no long `while(...)` loops)
- Prefer `millis()`-based scheduling for animations / timeouts
- Keep commands within `0..99` and document them in `docs/PROTOCOL.md`

## PR checklist
- Update `docs/PROTOCOL.md` if you add or change commands
- Add at least one example command to `examples/commands.txt`
