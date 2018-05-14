# Concurrency-3

### Problem 1

Consider a sharable resource with the following characteristics:

1. As long as there are fewer than three processes using the resource, new processes can start using it right away.
2. Once there are three processes using the resource, all three must leave before any new processes can begin using it.

Implement a mutual exclusion solution that meets the above constraints.

### Problem 2

Three kinds of threads share access to a singly-linked list: searchers, inserters and deleters. Searchers merely examine the list; hence they can execute concurrently with each other. Inserters add new items to the end of the list; insertions must be mutually exclusive to preclude two inserters from inserting new items at about the same time. However, one insert can proceed in parallel with any number of searches. Finally, deleters remove items from anywhere in the list. At most one deleter process can access the list at a time, and deletion must also be mutually exclusive with searches and insertions.

Write concurrent code in any language you like for searchers, inserters and deleters that enforces this kind of three-way categorical mutual exclusion. Remember, some languages have the constructs for concurrency but do NOT actually execute in parallel. Be aware of this and choose wisely.