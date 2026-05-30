# Easy Ticket: SCOUT-7-002

## Source
/Users/olek/projects/llm/games/ctp2/.easy/../.scouts/tickets/H-container--ctp2_code-ctp-ctp2_utils-pointerlist-h--01.md

## Change Required
PointerList<T> is a hand-rolled linked list that duplicates std::list with unsafe raw-pointer semantics

## Current State
`PointerList<T>` is a template class that re-implements a doubly-linked list from scratch. It performs raw `new`/`delete` for every node, lacks move semantics, and provides a dangerous `DeleteAll()` method that destroys both the list node and the object it points to—making ownership semantics ambiguous and risking double-delete.

## Code Evidence
```cpp
// File: ctp2_code/ctp/ctp2_utils/pointerlist.h
// Lines: 80-93
	virtual ~PointerList()
	{
		while (m_head)
        {
			PointerListNode * node = m_head;
			m_head = m_head->m_next;
			delete node;
		}
	};
```

```cpp
// File: ctp2_code/ctp/ctp2_utils/pointerlist.h
// Lines: 95-106
	void DeleteAll()
	{
		while (m_head)
        {
			PointerListNode * node = m_head;
			m_head = m_head->m_next;
			delete node->m_obj;    // <-- destroys owned objects
			delete node;
		}
		m_tail = NULL;
		m_count = 0;
	};
```

```cpp
// File: ctp2_code/ctp/ctp2_utils/pointerlist.h
// Lines: 256-266
template <class T> void PointerList<T>::AddTail(T *obj)
{
	PointerListNode* node = new PointerListNode(obj);
	...
}
```

## Suggested Direction
Replace `PointerList<T>` with `std::list<T*>` (or `std::list<std::unique_ptr<T>>` if the list owns the objects). The `Walker` inner class can be replaced with standard iterators. If deletion semantics are needed, use `std::list<std::unique_ptr<T>>` and let the container manage lifetime automatically.

## Files You May Edit
ctp2_code/ctp/ctp2_utils/pointerlist.h

## Acceptance Criteria (ALL must pass)
1. Build passes: `mise exec -- meson compile -C build ctp2_fast_tests`
2. Fast tests pass: `./build/ctp2_fast_tests`
3. Diff touches ONLY the files listed above
4. No new compiler warnings
5. Commit with conventional commit message

## Commit (REQUIRED)
```bash
git add ctp2_code/ctp/ctp2_utils/pointerlist.h
git commit -m "refactor(ctp): replace C arrays with std::array"
```
