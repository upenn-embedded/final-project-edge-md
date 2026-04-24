---
title: "Verification before velocity"
date: 2025-04-01
description: "Why I reach for testbenches, formal tools, and corner cases before declaring a design done."
tags:
  - rtl
  - verification
---

## The default stance

In student and hobby projects it is tempting to tune frequency or area first. In practice, **confidence** is the bottleneck: without a harness that exercises real cases, you do not know what you shipped.

## What that looks like

- Directed tests for bring-up and debuggability.
- Constrained random or property-style checks when the state space explodes.
- Formal or equivalence checks when the spec is crisp enough to encode.

That mix changes with the project — the point is to pick tools deliberately instead of stopping at “it simulates once.”
