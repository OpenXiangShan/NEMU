# xxHash

Unmodified `xxhash.c`, `xxhash.h` and `LICENSE` from
[xxHash v0.8.4](https://github.com/Cyan4973/xxHash/tree/v0.8.4), commit
`c87183a77d67f7d37e3d2d1b7eaac5e7c695e4f0`.

Built into NEMU when `FAST_REF` or `STORE_LOG_HASH` is enabled, with the
`nemu_xxh_` symbol prefix. No system `libxxhash` is required.

This component uses BSD 2-Clause, not NEMU's Mulan PSL v2. Preserve the upstream
notices; binary distributions must include the copyright, license conditions
and disclaimer in the accompanying [LICENSE](LICENSE).
