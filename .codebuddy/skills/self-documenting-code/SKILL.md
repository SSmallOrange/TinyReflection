---
name: self-documenting-code
description: >
  通过让代码自解释来消除冗余注释。适用于审查含解释性注释、魔法数、复杂条件表达式的代码，
  以及每当想要写一条描述"代码做了什么"的注释时触发。注释只解释"为什么"，不重述"做了什么"。
  中文触发词："注释精简"、"删除冗余注释"、"自文档化"、"self-documenting"。
---

# Self-Documenting Code（自文档化代码）

> 中文导读：本技能约束注释的"极简原则"——一条解释代码"做了什么"的注释是一次失败的
> 重构。注释会腐烂（没有编译器、测试或 linter 检查它们），而代码不会撒谎。凡是可以
> 用代码构造物表达的信息，都应该交给代码本身，注释只保留代码无法表达的内容（"为什么"）。
> 审查或编写代码时发现以下情况，即应用本技能。

A comment explaining *what* code does is a failed refactor. Comments rot — no compiler, test, or linter checks them, so they drift from the code and eventually lie; code cannot lie. Replace the comment with the construct it was compensating for:

- **Named constants** for magic values:
  ```ts
  if (status == 5) { ... }        // 5 means the message was sent
  if (status == MESSAGE_SENT) { ... }   // comment now redundant — delete it
  ```
- **Named intermediates and operations**, so the code reads as the comment would have:
  ```ts
  const userIsAuthor = message.authorId === user.id;
  const editWindowOpen = message.ageMinutes() < EDIT_WINDOW_MINUTES;
  if (userIsAuthor && editWindowOpen) { ... }
  ```
  Inline a one-use expression when a local name would merely restate its syntax. Keep a named local when it names a domain concept, records a snapshot, avoids repeated evaluation, or makes dense code easier to read. Extract a function only when it recurs, represents an independently meaningful domain operation or boundary, or lets a still-complex caller read as coherent happy-path steps. Length is not the test: a short helper earns its place when its trustworthy name tells the caller everything needed at that level of abstraction. If the reader must open it to discover behavior relevant to the caller, its name or the extraction is wrong. Never extract solely for isolated testing.
- **Types that encode the contract**: `unique_ptr` instead of "// caller must free"; `Optional<Timestamp>` instead of "// -1 means not set"; `Duration` instead of "// in seconds". The compiler enforces types; nobody enforces comments.

Legitimate comments say *why*, which code cannot express: performance-motivated weirdness ("unrolled — profiler showed this loop is 40% of frame time"), links to the algorithm, paper, spec, or ticket the code implements, warnings about non-obvious hazards.

Passing this skill's tests is necessary, not sufficient: a comment can restate nothing and carry genuine rationale yet still be unintelligible, because its nouns ("the caller", "the guard") refer to things only the author's conversation named, or it narrates how *other files* use this code. Every comment that survives here must also pass the stranger test in `writing-comments` — flag undecodable rationale as a defect exactly like redundancy.

API documentation is a different thing from comments: docstrings on public interfaces describing the contract — expected states, error conditions, thread safety — are for *users* who shouldn't have to read the implementation. Keep those, adjacent to the code so they stay in sync (Doxygen/pydoc/JavaDoc style).

Flag every remaining comment that merely restates its code.
