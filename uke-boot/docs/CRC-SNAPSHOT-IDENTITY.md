# Immutable GPT maintenance snapshot identity

6 November 2026. After a successful 12214/B boot, the packaged startup service
refuses CRC maintenance because an existing durable record differs. Automatic
staging is enabled and the local rollback preference remains off. The refusal
occurs before the maintenance commit; deleting the old record would hide the
underlying identity collision.

Version 101 names a CRC-only record using the partition-array digest. Different
observed backup headers can accompany identical arrays. The immutable journal
then correctly refuses reuse of the same filename with different contents.
Version 102 keys that specific journal by the digest of its complete observed
MBR, primary header, backup header and partition-array bytes. Serialization and
identity use the same bounded byte builder. Legacy records stay untouched;
existing same-name records must still match exactly. Other update/rollback
journals keep their existing identities and refusal rules.

The full 101 source capsule and all original package checks are retained.
Actual regular-file GPT tests cover 512/4096-byte sectors, both selected slots,
two different stale headers with identical arrays, a return to an earlier
snapshot, retained legacy records, same-name content corruption, read-only
preview and unchanged unapproved state. GCC/Clang and sanitizer modes precede
two assertion mutants per compiler. Only native COPR performs compilation.

This is a project-owned journal identity correction. It changes no GPT
geometry, flags, GUIDs, partition payloads, UFS selection, acceptance semantics
or firmware algorithm. The exact Qualcomm ABL comparison and its limits remain
in ROLLBACK-OFF-CRC.md; no donor code or hardware value is transplanted. A target
DNF update, independent array/image/preference comparison and a successful
packaged startup service remain required before claiming recovery.
