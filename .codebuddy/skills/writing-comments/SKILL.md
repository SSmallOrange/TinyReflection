---
name: writing-comments
description: >
  如何写出一个陌生人能看懂的注释/docstring/文档注释。适用于编写或修改任何注释时——
  尤其是函数和类型的文档注释——以及审查现有注释的模糊性、悬空指代或 AI 腔措辞时触发。
  是 self-documenting-code 的配套技能（陌生人测试）。
---

# Writing Comments

Write every comment for exactly one reader: a competent developer seeing this file for the first time, with no access to the pull request, the conversation that produced the code, other files, or the author's head. These rules are distilled from the best-commented codebases in the wild — Go's standard library, SQLite (whose source is ~35% comments explaining intent), Redis, Rust's std — which independently converge on the same conventions.

**The stranger test (master rule).** Cover the function body. Hand the comment plus the signature to someone who has never seen this repository. They must be able to say what the thing does, and every noun in the comment must resolve to a parameter, a named identifier in the signature, or a term the comment itself defines. A noun that needs the PR, the chat, or another file to decode fails the test.

1. **Start with the name.** The first sentence names the thing and states what it does, in a complete present-tense sentence. Booleans use Go's fixed verb: *reports whether*.
   ```ts
   // BAD:  True when the caller asked to resume.          (who is "the caller"?)
   // GOOD: canRetry reports whether job's retry budget allows another attempt.
   ```
   Functions: third-person verb — *returns*, *parses*, *creates* ("Marshal returns the JSON encoding of v"). Types: a noun phrase ("A fixed-size ring buffer of recent log lines."). A comment forced to begin with the identifier cannot have a mystery subject.

2. **Nouns come from the signature.** Refer to parameters and fields by their names — `requested`, `account.freezeReason` — never by role-words invented for the comment ("the caller", "the guard", "the retry path"). If the comment needs a concept the code doesn't name, that's a naming gap: name it in code first. **Resolution is not comprehension**: pointing a noun at an existing identifier passes only if that identifier would itself survive the `naming` skill's stranger check — resolving "the override" to `resolveBillingFreezeOverride` merely relocates the mystery when nothing anywhere says what is overridden. When the honest fix is a rename that ripples through call sites, report the naming gap as a finding with its blast radius instead of silently settling for prose; the churn budget is the human's call.

3. **Plain words; mechanism, not metaphor.** Never assert importance in place of information. Telltale slop: "load-bearing", "the crux", "crucially", "subtle but important", "the X is what bounds/anchors/drives the Y", "note that". Each hides a concrete fact — state the fact:
   ```ts
   // BAD:  The order is load-bearing.
   // GOOD: flush() must run before close(): close() frees the buffer flush() writes to.
   ```
   Prefer everyday words over insider jargon ("check" over "predicate") when they say the same thing. Domain terms the codebase itself uses are fine.

4. **Describe the contract, not the call graph.** A doc comment states what this function does, requires, and guarantees — never who calls it or how other files use it. Naming another file ("checkout.ts uses this to…") goes stale on the next edit and forces the reader on a field trip. Worse, it usually marks real coupling: if two places must stay in agreement, fix the code — one shared function, or the invariant enforced at the single site that takes the lock — instead of posting prose warnings (see `misuse-resistance`).

5. **Timeless present tense; no review talk.** The comment documents the code as it now is. Rationale for the change ("shared with the caller that takes the guard, so…"), comparisons to the old version ("now", "instead of", "previously"), and justifications addressed to a reviewer belong in the commit message or PR description, not the source.

6. **A why-comment carries its whole story.** State condition and consequence concretely inside the comment — "we re-read the length here because a concurrent XDEL may have shrunk the listpack" — not a gesture at shared context. If the explanation comes out vague or defensive, treat that as a signal the design needs work, not the wording (Redis practice: writing the comment is how you audit the code).

7. **Never imitate bad local idiom.** Matching surrounding comment style does not extend to matching its vagueness. Existing elliptical or slop comments in the file are defects to flag, not templates.

After these rules, most short functions need one sentence or nothing — a one-line predicate whose body restates the comment should usually lose the comment (see `self-documenting-code`).
